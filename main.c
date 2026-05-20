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
#include <debug.h>

#define dprintf(args...) \
    do { \
        scr_printf(args); \
        printf(args); \
    } while (0)
#else
#define dprintf(args...) \
    do { \
        printf(args); \
    } while (0)
#endif

#define PKGI_USER_AGENT "Mozilla/5.0 (PLAYSTATION 3; 1.00)"

#ifdef _EE
static const char *eeip_event_name(enum EEIP_PROGRESS_EVENT ev) {
    switch (ev) {
        case EEIP_PROGRESS_SETTING_LINK_MODE:  return "setting link mode";
        case EEIP_PROGRESS_TCPIP_INIT:         return "lwIP init";
        case EEIP_PROGRESS_APPLYING_IP_CONFIG: return "applying IP config";
        case EEIP_PROGRESS_WAITING_LINK_UP:    return "waiting for link up";
        case EEIP_PROGRESS_LINK_UP:            return "link up";
        case EEIP_PROGRESS_WAITING_DHCP:       return "waiting for DHCP lease";
        case EEIP_PROGRESS_DHCP_BOUND:         return "DHCP bound";
        case EEIP_PROGRESS_READY:              return "ready";
    }
    return "?";
}

static void on_net_progress(enum EEIP_PROGRESS_EVENT ev, void *user) {
    (void)user;
    dprintf("[net] %s\n", eeip_event_name(ev));
}

static int network_init(void) {
    eeip_network_config_t cfg;
    eeip_network_config_default_dhcp(&cfg);
    cfg.on_progress = on_net_progress;

    enum EEIP_NET_STATUS rc = configure_eeip_network(&cfg);
    if (rc != EEIP_NET_STATUS_OK) {
        dprintf("Network configuration failed: %d\n", rc);
        return -1;
    }

    struct ip4_addr ip, nm, gw;
    if (eeip_get_current_config(&ip, &nm, &gw) == 0) {
        dprintf("IP:\t%d.%d.%d.%d\n",
            ip4_addr1(&ip), ip4_addr2(&ip), ip4_addr3(&ip), ip4_addr4(&ip));
    }

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
            if (file == NULL) {
                dprintf("Failed to reopen %s to measure size\n", nameFile);
            } else {
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
    }
    dprintf("Curl Example Finished\n");

#ifdef _EE
    deinit_drivers();

    SleepThread();
#endif

    return 0;
}