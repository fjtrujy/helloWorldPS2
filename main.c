#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/param.h>

#include <curl/curl.h>

#ifdef _EE
#include <kernel.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <sbv_patches.h>

#include <ps2_network_driver.h>
#include <netman.h>
#include <ps2ip.h>
#include <debug.h>

#define dprintf(args...) \
    scr_printf(args); \
    printf(args);
#else
#define dprintf(args...) \
    printf(args);
#endif

#define PKGI_USER_AGENT "Mozilla/5.0 (PLAYSTATION 3; 1.00)"

#ifdef _EE
static int ethGetNetIFLinkStatus(void)
{
	return (NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP);
}

static int ethApplyNetIFConfig(int mode)
{
    int result;
    // By default, auto-negotiation is used.
    static int CurrentMode = NETMAN_NETIF_ETH_LINK_MODE_AUTO;

    if (CurrentMode != mode)
    { // Change the setting, only if different.
        if ((result = NetManSetLinkMode(mode)) == 0)
            CurrentMode = mode;
    }
    else
        result = 0;

    return result;
}

static int WaitValidNetState(int (*checkingFunction)(void))
{
	int retry_cycles;

	// Wait for a valid network status;
	for (retry_cycles = 0; checkingFunction() == 0; retry_cycles++)
	{ // Sleep for 1000ms.
		usleep(1000 * 1000);
        dprintf(".");

		if (retry_cycles >= 10) // 10s = 10*1000ms
			return -1;
	}

	return 0;
}

static int ethWaitValidNetIFLinkState(void)
{
	return WaitValidNetState(&ethGetNetIFLinkStatus);
}

static int ethApplyIPConfig(int use_dhcp, const struct ip4_addr* ip, const struct ip4_addr* netmask, const struct ip4_addr* gateway)
{
    t_ip_info ip_info;
    int result;

    // SMAP is registered as the "sm0" device to the TCP/IP stack.
    if ((result = ps2ip_getconfig("sm0", &ip_info)) >= 0)
    {
        // Check if it's the same. Otherwise, apply the new configuration.
        if ((use_dhcp != ip_info.dhcp_enabled) || (!use_dhcp &&
                                                        (!ip_addr_cmp(ip, (struct ip4_addr*)&ip_info.ipaddr) ||
                                                            !ip_addr_cmp(netmask, (struct ip4_addr*)&ip_info.netmask) ||
                                                            !ip_addr_cmp(gateway, (struct ip4_addr*)&ip_info.gw))))
        {
            if (use_dhcp)
            {
                ip_info.dhcp_enabled = 1;
            }
            else
            { // Copy over new settings if DHCP is not used.
                ip_addr_set((struct ip4_addr*)&ip_info.ipaddr, ip);
                ip_addr_set((struct ip4_addr*)&ip_info.netmask, netmask);
                ip_addr_set((struct ip4_addr*)&ip_info.gw, gateway);

                ip_info.dhcp_enabled = 0;
            }

            // Update settings.
            result = ps2ip_setconfig(&ip_info);
        }
        else
            result = 0;
    }

    return result;
}

static void ethdprintIPConfig(void)
{
    t_ip_info ip_info;
    u8 ip_address[4], netmask[4], gateway[4];

    // SMAP is registered as the "sm0" device to the TCP/IP stack.
    if (ps2ip_getconfig("sm0", &ip_info) >= 0)
    {
        // Obtain the current DNS server settings.

        ip_address[0] = ip4_addr1((struct ip4_addr*)&ip_info.ipaddr);
        ip_address[1] = ip4_addr2((struct ip4_addr*)&ip_info.ipaddr);
        ip_address[2] = ip4_addr3((struct ip4_addr*)&ip_info.ipaddr);
        ip_address[3] = ip4_addr4((struct ip4_addr*)&ip_info.ipaddr);

        netmask[0] = ip4_addr1((struct ip4_addr*)&ip_info.netmask);
        netmask[1] = ip4_addr2((struct ip4_addr*)&ip_info.netmask);
        netmask[2] = ip4_addr3((struct ip4_addr*)&ip_info.netmask);
        netmask[3] = ip4_addr4((struct ip4_addr*)&ip_info.netmask);

        gateway[0] = ip4_addr1((struct ip4_addr*)&ip_info.gw);
        gateway[1] = ip4_addr2((struct ip4_addr*)&ip_info.gw);
        gateway[2] = ip4_addr3((struct ip4_addr*)&ip_info.gw);
        gateway[3] = ip4_addr4((struct ip4_addr*)&ip_info.gw);


        dprintf("IP:\t%d.%d.%d.%d\n",
            ip_address[0], ip_address[1], ip_address[2], ip_address[3],
            netmask[0], netmask[1], netmask[2], netmask[3],
            gateway[0], gateway[1], gateway[2], gateway[3]);
    }
    else
    {
        dprintf("Unable to read IP address.\n");
    }
}

static int ethGetDHCPStatus(void)
{
	t_ip_info ip_info;
	int result;

	if ((result = ps2ip_getconfig("sm0", &ip_info)) >= 0)
	{ // Check for a successful state if DHCP is enabled.
		if (ip_info.dhcp_enabled)
			result = (ip_info.dhcp_status == DHCP_STATE_BOUND || (ip_info.dhcp_status == DHCP_STATE_OFF));
		else
			result = -1;
	}

	return result;
}

static int ethWaitValidDHCPState(void)
{
	return WaitValidNetState(&ethGetDHCPStatus);
}

static int network_init() {
    struct ip4_addr *IP, *NM, *GW;

    // Using DHCP
    IP = malloc(sizeof(struct ip4_addr));
    NM = malloc(sizeof(struct ip4_addr));
    GW = malloc(sizeof(struct ip4_addr));

    // The DHCP server will provide us this information.
    ip4_addr_set_zero(IP);
    ip4_addr_set_zero(NM);
    ip4_addr_set_zero(GW);

    // The network interface link mode/duplex can be set.
    int EthernetLinkMode = NETMAN_NETIF_ETH_LINK_MODE_AUTO;

    // Attempt to apply the new link setting.
    dprintf("Setting link mode...");
    if (ethApplyNetIFConfig(EthernetLinkMode) != 0)
    {
        dprintf("\nError: failed to set link mode.\n");
        free(IP);
        free(NM);
        free(GW);
        return -1;
    }
    dprintf("done!\n");

    // Initialize the TCP/IP protocol stack.
    ps2ipInit(IP, NM, GW);

    ethApplyIPConfig(1, IP, NM, GW);
    // Wait for the link to become ready.
    dprintf("Waiting for connection...");
    int ethState = ethWaitValidNetIFLinkState();
    dprintf("done!\n");
    free(IP);
    free(NM);
    free(GW);
    
    if (ethState != 0)
    {
        dprintf("Error: failed to get valid link status.\n");
        return -2;
    }

    dprintf("Waiting for DHCP lease...");
    if (ethWaitValidDHCPState() != 0)
    {
        dprintf("DHCP failed\n.");
        return -3;
    }
    dprintf("done!\n");
    dprintf("Initialized:\n");

    ethdprintIPConfig();

    return 0;
}

static void reset_IOP() {
    SifInitRpc(0);
    /* Comment this line if you don't wanna debug the output */
    while (!SifIopReset(NULL, 0)) {};

    while (!SifIopSync()) {};
    SifInitRpc(0);
    sbv_patch_enable_lmb();
}

static void init_drivers() {
    init_ps2_filesystem_driver();
    init_network_driver(true);
}

static void deinit_drivers() {
    deinit_network_driver(true);
    deinit_ps2_filesystem_driver();
}

#endif

// Callback function to track progress
int bar_state = 0;
int latest_round = -1;
int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    int round = bar_state / 100;
#if !defined(_EE)
    switch (round)
    {
    case 0:
        dprintf("|");
        break;
    case 1:
        dprintf("/");
        break;
    case 2:
        dprintf("-");
        break;
    case 3:
        dprintf("\\");
        break;
    default:
        break;
    }
    dprintf("\r");
#endif
    if (round != latest_round) {
#if defined(_EE)
        dprintf(".");
#endif
        latest_round = round;
    }
    bar_state = (bar_state + 1) % 400;

    return 0;
}

size_t downloaded = 0;
int y = 0;
// Callback function to handle received data
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t nmemb_written, total_written;
    nmemb_written = fwrite(ptr, size, nmemb, (FILE *)userdata);
    total_written = nmemb_written * size;
    downloaded += total_written;
#if defined(_EE)
    if (y  == 0) {
        y = scr_getY();
    }
    scr_clearline(y);
    scr_setXY(0, y);
    scr_printf("Downloaded: %ld bytes", downloaded);
#endif
    if (nmemb_written != nmemb) {
        dprintf("\nError writing file requested %ld items, written %ld items\n", nmemb, nmemb_written);
    }
    return total_written;
}

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *clientp)
{
  const char *text;
  (void)handle; /* prevent compiler warning */
  (void)clientp;

  // Write data in a file called log.txt
    FILE *log = fopen("log.txt", "a");
    if (log) {
        fwrite(data, size, 1, log);
        fclose(log);
    }
 
 dprintf("==================\n");
 dprintf("%s\n", data);
 dprintf("==================\n");
  return 0;
}

int main(int argc, char **argv) {
    CURL *curl;
    CURLcode res;
    // const char *nameFile = "GRIDnjgCNVNWExNcSeHMBxJUBsDmwjXBHNqqwHQnnLhwCVQeRBjCoViIhGvLQSQA.pkg";
    // const char *url = "http://zeus.dl.playstation.net/cdn/EP9000/UCES01421_00/GRIDnjgCNVNWExNcSeHMBxJUBsDmwjXBHNqqwHQnnLhwCVQeRBjCoViIhGvLQSQA.pkg";
    // const char *nameFile = "ubuntu-22.04-beta-preinstalled-server-riscv64+unmatched.img.xz";
    // const char *url = "https://old-releases.ubuntu.com/releases/jammy/ubuntu-22.04-beta-preinstalled-server-riscv64+unmatched.img.xz"; // 700 MB
    // const char *nameFile = "ubuntu-22.04.3-desktop-amd64.iso.zsync";
    // const char *url = "https://old-releases.ubuntu.com/releases/jammy/ubuntu-22.04.3-desktop-amd64.iso.zsync"; // 11 MB
    // const char *nameFile = "sitemap.xml";
    // const char *url = "http://bucanero.com.ar/sitemap.xml";
    const char *nameFile = "github.html";
    const char *url = "https://github.com";
    

#ifdef _EE
    
    init_scr();
    dprintf("\n\n\nStarting Curl Example...\n");
    reset_IOP();
    init_drivers();
    dprintf("Drivers initialized\n");

    if (network_init() != 0) {
        dprintf("Failed to initialize network\n");
        deinit_drivers();
        return -1;
    }

    // chdir("mass:/curl");
#endif

    // Initialize libcurl
    curl = curl_easy_init();
    if(curl) {
        FILE *fp = fopen(nameFile, "wb"); // Open file for writing
        if (fp == NULL) {
            dprintf("Failed to open file for writing\n");
#if defined(_EE)
            deinit_drivers();
            SleepThread();
#endif
            return 1;
        }

        // Set verbose output
        // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
 
        /* the DEBUGFUNCTION has no effect until we enable VERBOSE */
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        
        // Set the URL you want to retrieve
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Set the write callback function
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // Set the progress callback function
        // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        // curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);

        // Set user agent string
        curl_easy_setopt(curl, CURLOPT_USERAGENT, PKGI_USER_AGENT);
        // don't verify the certificate's name against host
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        // don't verify the peer's SSL certificate
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        // Set SSL VERSION to TLS 1.2
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        // Set timeout for the connection to build
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        // maximum number of redirects allowed
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 20L);
        // Fail the request if the HTTP code returned is equal to or larger than 400
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        // request using SSL for the FTP transfer if available
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);

        // Perform the request
        dprintf("Downloading file...\n");
        clock_t start = clock();
        res = curl_easy_perform(curl);
        clock_t end = clock();
        dprintf("\n");
        if(res != CURLE_OK)
            dprintf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        // Cleanup

        curl_easy_cleanup(curl);
        fclose(fp); // Close the file

        if(res == CURLE_OK) {
            // Check file size and calculate download speed
            FILE *file = fopen(nameFile, "rb");
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fclose(file);

            scr_clear();
            dprintf("\n\n\n\n");
            double time_taken = ((double)(end - start)) / 1000000;
            double download_speed = (file_size / 1024) / time_taken;
            dprintf("Downloaded %ld bytes in %.2f seconds\n", file_size, time_taken);
            dprintf("Download speed: %.2f KB/s\n", download_speed);
        }
    }
    dprintf("Curl Example Finished\n");

#ifdef _EE
    deinit_drivers();

    SleepThread();
#endif

    return 0;
}