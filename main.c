#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <curl/curl.h>

#ifdef _EE
#include <kernel.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <sbv_patches.h>

#include <ps2_network_driver.h>
#include <netman.h>
#include <ps2ip.h>

#define dprintf(args...) \
    scr_printf(args); \
    printf(args);
#else
#define dprintf(args...) \
    printf(args);
#endif

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
	int ThreadID, retry_cycles;

	// Wait for a valid network status;
	for (retry_cycles = 0; checkingFunction() == 0; retry_cycles++)
	{ // Sleep for 1000ms.
		usleep(1000 * 1000);

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
    if (ethApplyNetIFConfig(EthernetLinkMode) != 0)
    {
        dprintf("Error: failed to set link mode.\n");
        free(IP);
        free(NM);
        free(GW);
        return -1;
    }

    // Initialize the TCP/IP protocol stack.
    ps2ipInit(IP, NM, GW);

    ethApplyIPConfig(1, IP, NM, GW);
    // Wait for the link to become ready.
    dprintf("Waiting for connection...\n");
    int ethState = ethWaitValidNetIFLinkState();
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
#if !defined(DEBUG) || defined(BUILD_FOR_PCSX2)
    /* Comment this line if you don't wanna debug the output */
    while (!SifIopReset(NULL, 0)) {};
#endif

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
int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    // Calculate progress percentage
    double progress = (dlnow > 0) ? (dlnow / (double)dltotal) * 100.0 : 0.0;
    
    // Display progress information
#if defined(_EE)
    int y = scr_getY();
    scr_setXY(0, y);
    scr_clearline(y);
#endif
    dprintf("Progress: %.2f%%", progress);

    return 0;
}

// Callback function to handle received data
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = size * nmemb;
    fwrite(ptr, size, nmemb, (FILE *)userdata);
    return total_size;
}

int main(int argc, char **argv) {
    CURL *curl;
    CURLcode res;

#ifdef _EE
    init_scr();
    scr_printf("\n\nStarting Curl Example...\n");

    reset_IOP();
    init_drivers();

    if (network_init() != 0) {
        deinit_drivers();
        return -1;
    }
#endif

    // Initialize libcurl
    curl = curl_easy_init();
    if(curl) {
        // Set verbose output
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        
        // Set the URL you want to retrieve
        curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");

        // Set the write callback function
        FILE *fp = fopen("output.html", "wb"); // Open file for writing
        if (fp == NULL) {
            dprintf("Failed to open file for writing\n");
            return 1;
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // Set the progress callback function
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);

        // Perform the request
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            dprintf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        // Cleanup
        curl_easy_cleanup(curl);
        fclose(fp); // Close the file
    }
    dprintf("Curl Example Finished\n");

#ifdef _EE
    deinit_drivers();

    SleepThread();
#endif

    return 0;
}