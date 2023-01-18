/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"

#include "esp_netif.h"
//#include "protocol_examples_common.h"
#include <lwip/netdb.h>
//#include "freertos/timers.h"
#include "message.h"
#include "engine.h"
#include "httpserver.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL 7
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

#define PORT 111

static const char *TAG = "GRID Main.c";
esp_netif_t *netif, *netif_sta;

message_t messageClientOut, messageClientIn, messageIn;

wifi_config_t wifi_config_sta;

int busyCnt = 0;

typedef struct networkStatus
{
    char *internetSSID;
    char *internetPassword;
    char *gridSSID;
    int internetRSSI;
    int meshRSSI;
    uint8_t IAmRoot;
} networkStatus_t;

networkStatus_t networkStatus = {
    .internetSSID = "DNET Fast",
    .internetPassword = "andrewhooban92126",
    .gridSSID = "grid",
    .internetRSSI = -999,
    .IAmRoot = 0};

char stationIPAddr[64] = "not ready";
int * stationRSSI = &(networkStatus.internetRSSI);

//TimerHandle_t xTimerSocket;
//int listen_sock;

// Global string used to hold ip Address returned by ipString()
char ipString[20]; // global string to hold conversion

//Rick's function toconvert the ESP hex address to a global string
char *toIpString(uint32_t hexIpAddr)
{
    uint8_t ip0, ip1, ip2, ip3;
    ip0 = hexIpAddr >> 24;
    ip1 = (0x00ff0000 & hexIpAddr) >> 16;
    ip2 = (0x0000ff00 & hexIpAddr) >> 8;
    ip3 = (0x000000ff & hexIpAddr);

    snprintf(ipString, sizeof(ipString), "%d.%d.%d.%d", ip3, ip2, ip1, ip0);
    return ipString;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

//void vTimerCallback( TimerHandle_t pxTimer ) {
//    printf("\nvTimerCalledback called\n");
//}

static void do_retransmit(const int sock)
{
    int len;

    ESP_LOGI(TAG, "Waiting at recv");
    len = recv(sock, &messageIn, sizeof(messageIn), 0);
    if (len < 0)
    {
        ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
    }
    else if (len == 0)
    {
        ESP_LOGW(TAG, "Connection closed");
    }
    else if (len != sizeof(messageIn))
    {
        ESP_LOGE(TAG, "Error message length wrong %d", len);
        return;
    }
    else
    {
        //ESP_LOGI(TAG, "Rxd %d bytes", len);
        printMessage(&messageIn);

        int written = send(sock, &messageIn, sizeof(messageIn), 0);
        if (written < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            return;
        }
        ESP_LOGI(TAG, "Txd %d bytes", written);
    }
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters; // IPV4 or IPV6
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    //    xTimerSocket = xTimerCreate(    "Timer",       // Just a text name, not used by the kernel.
    //                                        ( 4000 / portTICK_PERIOD_MS  ),      // The timer period in ticks.
    //                                        pdFALSE,       // The timers will auto-reload themselves when they expire.
    //                                       ( void * ) 1,  // Assign each timer a unique id equal to its array index.
    //                                        vTimerCallback // Each timer calls the same callback when it expires.
    //                                    );

    if (addr_family == AF_INET)
    {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY); // from Any address / interface
        dest_addr_ip4->sin_family = AF_INET;                // IPV4
        dest_addr_ip4->sin_port = htons(PORT);              // Listen Port
        ip_protocol = IPPROTO_IP;                           // IP
    }
    else if (addr_family == AF_INET6)
    {
        bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1)
    {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        struct timeval to;
        to.tv_sec = 5;
        to.tv_usec = 0;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) < 0)
        {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
        }

        // Convert ip address to string
        if (source_addr.ss_family == PF_INET)
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        else if (source_addr.ss_family == PF_INET6)
        {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }

        u16_t port;
        port = ((struct sockaddr_in *)&source_addr)->sin_port;

        ESP_LOGI(TAG, "Socket accepted ip address: %s, port: %d", addr_str, port);

        esp_netif_dhcp_status_t status = 33;               //testing
        esp_err_t code;                                    //testing
        code = esp_netif_dhcps_get_status(netif, &status); //testing
        printf("code = %d\n", code);                       //testing
        printf("status = %d\n", status);                   //testing

        //        xTimerStart(xTimerSocket, 0 );

        do_retransmit(sock);
        //xTimerStop(xTimerSocket, 0);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

// Initialize the ESP device as a Access Point
void wifi_init_softap(void)
{

    ESP_LOGI(TAG, "Starting wifi_init_softap()");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    netif = esp_netif_create_default_wifi_ap();
    netif_sta = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    //#define STATION_SSID "mesh"
    wifi_config_t wifi_config_ap = {
        .ap = {
            // .ssid = STATION_SSID,
            // .ssid_len = strlen(STATION_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK},
        /* .sta = {
            .ssid = "dnet",
            .password = "andrewhooban92126",
            
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },*/
    };

    strncpy((char *)wifi_config_ap.ap.ssid, networkStatus.gridSSID, sizeof(wifi_config_ap.ap.ssid));

    strncpy((char *)wifi_config_sta.sta.ssid, networkStatus.gridSSID, sizeof(wifi_config_sta.sta.ssid));
    strncpy((char *)wifi_config_sta.sta.password, "mypassword", sizeof(wifi_config_sta.sta.password));

    wifi_config_sta.sta.bssid[0] = 0x84;
    wifi_config_sta.sta.bssid[1] = 0xcc;
    wifi_config_sta.sta.bssid[2] = 0xa8;
    wifi_config_sta.sta.bssid[3] = 0x59;
    wifi_config_sta.sta.bssid[4] = 0x6a;
    wifi_config_sta.sta.bssid[5] = 0xa9;
    wifi_config_sta.sta.bssid_set = 1;
    wifi_config_sta.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config_sta.sta.pmf_cfg.capable = true;
    wifi_config_sta.sta.pmf_cfg.required = false;

    /*
    wifi_config.sta.ssid = "dnet";
    wifi_config.sta.password = "andrewhooban92126";
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    */

    //if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
    //    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    //}

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
    ESP_ERROR_CHECK(esp_wifi_start());
    //esp_wifi_connect();

    esp_netif_dhcp_status_t status = 33;
    esp_err_t code;
    code = esp_netif_dhcps_get_status(netif, &status);
    printf("code = %d\n", code);
    printf("status = %d\n", status);

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif, &ip_info);
    char text[100];
    esp_ip4addr_ntoa(&(ip_info.ip), text, sizeof(text));
    printf("ip: %s\n", text);
    esp_ip4addr_ntoa(&(ip_info.gw), text, sizeof(text));
    printf("ip: %s\n", text);
    esp_ip4addr_ntoa(&(ip_info.netmask), text, sizeof(text));
    printf("ip: %s\n", text);

    //printf("status 1 = %d", netif->dhcps_status );
    esp_netif_dhcps_stop(netif);

    int subnet = 10;
    if (networkStatus.IAmRoot == 0)
        subnet = 11;
    esp_netif_set_ip4_addr(&(ip_info.ip), 10, 10, subnet, 100);
    esp_netif_set_ip4_addr(&(ip_info.gw), 10, 10, subnet, 100);
    esp_netif_set_ip_info(netif, &ip_info);

    // Get Stations IP adress
    esp_netif_get_ip_info(netif, &ip_info);
    esp_ip4addr_ntoa(&(ip_info.ip), text, sizeof(text));
    printf("\ngetting Stations IP address in function wifi_init_softap() \n");
    printf("wifi_init_softap() ip_info.ip ip address: %s\n", text);

    // Get Station's gateway
    esp_ip4addr_ntoa(&(ip_info.gw), text, sizeof(text));
    printf("wifi_init_softap() ip_info.ip gw address:: %s\n", text);
    esp_ip4addr_ntoa(&(ip_info.netmask), text, sizeof(text));

    esp_netif_dhcps_start(netif);

    //esp_netif_dhcps_option(netif, ESP_NETIF_OP_GET, );

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);

    ESP_LOGI(TAG, "end of  wifi_init_softap()");

} //end of  wifi_init_softap()

#define CONFIG_EXAMPLE_IPV4

//#define HOST_IP_ADDR "192.168.0.100"
//#define PORT 111

static void tcp_client_task(void *pvParameters)
{
    //char host_ip[] = HOST_IP_ADDR;
    esp_netif_ip_info_t ip_info;
    int addr_family = 0;
    int ip_protocol = 0;

    //vTaskDelay(4000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "trying to send from client to server\n");

    while (1)
    {
        ESP_LOGI(TAG, "restarting loop in tcp_client_task() ");
        ESP_LOGI(TAG, "trying to send from client to server");

        while (1)
        {
            wifi_ap_record_t apInfo;
            int apInfoReturnVal;
            apInfoReturnVal = esp_wifi_sta_get_ap_info(&apInfo);
            ESP_LOGI(TAG, "apInfoReturnVal = %d", apInfoReturnVal);

            if (apInfoReturnVal == ESP_OK)
            {

                esp_netif_get_ip_info(netif_sta, &ip_info);

                ESP_LOGI(TAG, "ssid = %s gw: %x", apInfo.ssid, ip_info.gw.addr);

                break;
            }
            else if (apInfoReturnVal == ESP_ERR_WIFI_CONN)
            {
                ESP_LOGI(TAG, "The station interface don’t initialized\n");
            }
            else if (apInfoReturnVal == ESP_ERR_WIFI_NOT_CONNECT)
            {
                ESP_LOGI(TAG, "The station is in disconnect status\n");
                esp_err_t err = esp_wifi_disconnect();
                ESP_LOGI(TAG, "err: %d\n", err);
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
                err = esp_wifi_connect();
                ESP_LOGI(TAG, "err: %d\n", err);
                if (err)
                    ESP_LOGI(TAG, "connect failed error code: %d\n", err);
                vTaskDelay(3000 / portTICK_PERIOD_MS);
            }
        }

#if defined(CONFIG_EXAMPLE_IPV4)
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = ip_info.gw.addr; //inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
        struct sockaddr_in6 dest_addr = {0};
        inet6_aton(host_ip, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        struct sockaddr_in6 dest_addr = {0};
        ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr));
#endif
        int sock = lwip_socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            ESP_LOGE(TAG, "    \"File table overflow\"");

            break;
        }

        //        ESP_LOGI(TAG, "Socket created with Id: %d, connecting to %s:%d", sock, host_ip, PORT);
        ESP_LOGI(TAG, "Socket created with Id: %d, connecting to %s:%d", sock, toIpString(ip_info.gw.addr), PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0)
        {
            ESP_LOGI(TAG, "Socket unable to connect because.... ");
            int err_numb = errno;

            if (err_numb == 113)
            {
                ESP_LOGE(TAG, "   \"No route to host\"");
            }
            else if (err_numb == EHOSTUNREACH)
            {
                ESP_LOGE(TAG, "   \"Host is unreachable\"");
            }
            else if (err_numb == ECONNREFUSED)
            {
                ESP_LOGE(TAG, "   \"Connection refused\"");
            }
            else if (err_numb == ECONNRESET)
            {
                ESP_LOGI(TAG, "   \"Connection reset by peer\"");
            }
            else
                ESP_LOGE(TAG, "    errno =  %d", err_numb);

            if (sock != -1)
            {
                ESP_LOGI(TAG, "Shutting down socket and restarting %d %d", shutdown(sock, 0), close(sock));
                //shutdown(sock, 0);
                //close(sock);
            }
            //esp_netif_dhcpc_stop(netif_sta);

            //ipCnt = (ipCnt + 1) % 10;

            //esp_netif_ip_info_t ip_info;
            //esp_netif_set_ip4_addr(&(ip_info.ip), 192, 168, 4, ipCnt + 80);
            //esp_netif_set_ip4_addr(&(ip_info.gw), 192, 168, 4, 100);
            //esp_netif_set_ip4_addr(&(ip_info.netmask), 255, 255, 255, 0);
            //esp_netif_set_ip_info(netif_sta, &ip_info);

            //esp_wifi_connect();

            vTaskDelay(600000 / portTICK_PERIOD_MS);
        }
        else
        {
            ESP_LOGI(TAG, "Successfully connected");

            ESP_LOGI(TAG, "Trying to send ...");
            //                int err = send(sock, payload, strlen(payload), 0);

            updateTimeStamp(&messageClientOut);
            int err = send(sock, &messageClientOut, sizeof(messageClientOut), 0);
            if (err < 0)
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            }

            //while (1)
            //{
            ESP_LOGI(TAG, "Waiting for responce ...");
            int len = recv(sock, &messageClientIn, sizeof(messageClientIn), 0);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
            }
            else if (len != sizeof(messageClientIn))
            {
                ESP_LOGE(TAG, "recv failed: message is not correct length %d", len);
            }
            // Data received
            //else if (len == 0) break;
            else
            {
                ESP_LOGI(TAG, "Received Message from Server");
                printMessage(&messageClientIn);
            }
            //}

            //vTaskDelay(1000 / portTICK_PERIOD_MS);

            if (sock != -1)
            {
                ESP_LOGI(TAG, "Shutting down socket and restarting...");
                shutdown(sock, 0);
                close(sock);
            }
            vTaskDelay(4000 / portTICK_PERIOD_MS);

            //esp_netif_dhcpc_stop(netif_sta);

            //ipCnt = (ipCnt + 1) % 10;

            //esp_netif_ip_info_t ip_info;
            //esp_netif_set_ip4_addr(&(ip_info.ip), 192, 168, 4, ipCnt + 80);
            //esp_netif_set_ip4_addr(&(ip_info.gw), 192, 168, 4, 100);
            //esp_netif_set_ip4_addr(&(ip_info.netmask), 255, 255, 255, 0);
            //esp_netif_set_ip_info(netif_sta, &ip_info);
        }
    }
    vTaskDelete(NULL);
}

int stationScan(void *pvParameters)
{

    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#esp32-wi-fi-scan

    wifi_scan_config_t scanConfig;
    int scanRetCode;

    int8_t bestRssi = -127;

    scanConfig.bssid = NULL;
    scanConfig.channel = 0;
    scanConfig.ssid = NULL;
    scanConfig.show_hidden = 0;
    scanConfig.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scanConfig.scan_time.active.max = 200;
    scanConfig.scan_time.active.min = 1000;

    if (pvParameters != NULL)
    {
        scanConfig.ssid = (uint8_t *)pvParameters;
    }

    printf("start Scan\n");
    scanRetCode = esp_wifi_scan_start(&scanConfig, true);
    printf("scanRetCode: %d\n", scanRetCode);

    uint16_t apNumber;
    esp_wifi_scan_get_ap_num(&apNumber);
    printf("number of AP found : %d\n", apNumber);

    const uint16_t maxNumAps = 10;
    wifi_ap_record_t apRecords[maxNumAps];
    uint16_t numAps = maxNumAps;
    esp_wifi_scan_get_ap_records(&numAps, apRecords);

    printf("%d APs found\n", numAps);
    for (int i = 0; i < numAps; i++)
    {
        if (i == 0)
            bestRssi = apRecords[i].rssi;
        printf("%d SSID: %s rssi: %d bssid: %02x:%02x:%02x:%02x:%02x:%02x\n", i, apRecords[i].ssid, apRecords[i].rssi,
               apRecords[i].bssid[0], apRecords[i].bssid[1], apRecords[i].bssid[2], apRecords[i].bssid[3], apRecords[i].bssid[4], apRecords[i].bssid[5]);
    }

    printf("Listing connected stations\n");
    wifi_sta_list_t sta;
    esp_wifi_ap_get_sta_list(&sta);
    printf("number of station connected is %d\n", sta.num);

    for (int i = 0; i < sta.num; i++)
    {
        printf("   station number %d, rssi: %d mac: %02x:%02x:%02x:%02x:%02x:%02x\n", i, sta.sta[i].rssi,
               sta.sta[i].mac[0], sta.sta[i].mac[1], sta.sta[i].mac[2], sta.sta[i].mac[3], sta.sta[i].mac[4], sta.sta[i].mac[5]);
    }

    //printf("Heap memory status\n");
    // heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);

    return bestRssi;
}

void stationScanner(void *params)
{
    while (1)
    {
        if (busyCnt <= 0) // do scan if not busy
        {
            networkStatus.internetRSSI = stationScan((void *)networkStatus.internetSSID);
            networkStatus.meshRSSI = stationScan((void *)"mesh");
            printf("stationScanner Network Status internet %d dBm, mesh %d dBm\n", networkStatus.internetRSSI, networkStatus.meshRSSI);

            if (networkStatus.meshRSSI == -127)
                networkStatus.IAmRoot = 1;
            else
                networkStatus.IAmRoot = 0;

            wifi_ap_record_t apInfo;
            int apInfoReturnVal;
            esp_netif_ip_info_t ip_info;
            bool connectedToCorrectStation = false;
            apInfoReturnVal = esp_wifi_sta_get_ap_info(&apInfo);
            printf("apInfoReturnVal = %d\n", apInfoReturnVal);

            if (networkStatus.IAmRoot)
            {

                printf("wifi_config_sta.sta.ssid   %s\n", wifi_config_sta.sta.ssid);
                printf("networkStatus.internetSSID %s\n", networkStatus.internetSSID);
                printf("apInfo.ssid                %s\n", apInfo.ssid);
                if (apInfoReturnVal == ESP_OK && strncmp((char *)wifi_config_sta.sta.ssid, networkStatus.internetSSID, sizeof(wifi_config_sta.sta.ssid)) == 0)
                {
                    connectedToCorrectStation = true;
                    esp_netif_get_ip_info(netif_sta, &ip_info);
                    uint8_t ip0, ip1, ip2, ip3;
                    ip0 = ip_info.gw.addr >> 24;
                    ip1 = (0x00ff0000 & ip_info.gw.addr) >> 16;
                    ip2 = (0x0000ff00 & ip_info.gw.addr) >> 8;
                    ip3 = (0x000000ff & ip_info.gw.addr);
                    printf("Root Station connection is valid. SSID = %s gw: %x\n", apInfo.ssid, ip_info.gw.addr);
                    printf("Root Station connection is valid. SSID = %s gw: %d.%d.%d.%d\n", apInfo.ssid, ip3, ip2, ip1, ip0);
                    toIpString(ip_info.ip.addr);
                    strncpy(stationIPAddr,ipString, sizeof(stationIPAddr));
                    printf("This node's ip address from station %s is ip_info.ip.addr : 0x%x (%s) \n",
                           apInfo.ssid, ip_info.ip.addr,stationIPAddr);
                }
                else
                {
                    printf("Fixing the SSIDs\n");
                    strncpy((char *)wifi_config_sta.sta.ssid, networkStatus.internetSSID, sizeof(wifi_config_sta.sta.ssid));
                    strncpy((char *)wifi_config_sta.sta.password, networkStatus.internetPassword, sizeof(wifi_config_sta.sta.password));
                    wifi_config_sta.sta.bssid_set = false;
                }
            }
            else // not root
            {
                if (apInfoReturnVal == ESP_OK && strncmp((char *)wifi_config_sta.sta.ssid, networkStatus.internetSSID, sizeof(wifi_config_sta.sta.ssid)) == 0)
                {
                    connectedToCorrectStation = true;
                    esp_netif_get_ip_info(netif_sta, &ip_info);
                    printf("Non Root Station connection is valid. SSID = %s gw: %x\n", apInfo.ssid, ip_info.gw.addr);
                }
                else
                {
                    strncpy((char *)wifi_config_sta.sta.ssid, networkStatus.gridSSID, sizeof(wifi_config_sta.sta.ssid));
                    strncpy((char *)wifi_config_sta.sta.password, "mypassword", sizeof(wifi_config_sta.sta.password));

                    wifi_config_sta.sta.bssid[0] = 0x84;
                    wifi_config_sta.sta.bssid[1] = 0xcc;
                    wifi_config_sta.sta.bssid[2] = 0xa8;
                    wifi_config_sta.sta.bssid[3] = 0x59;
                    wifi_config_sta.sta.bssid[4] = 0x6a;
                    wifi_config_sta.sta.bssid[5] = 0xa9;
                    wifi_config_sta.sta.bssid_set = 1;
                }
            }

            if (apInfoReturnVal == ESP_ERR_WIFI_CONN)
            {
                printf("The station interface did not initialized\n");
            }
            if (apInfoReturnVal == ESP_ERR_WIFI_NOT_CONNECT || connectedToCorrectStation == false)
            {
                printf("The station is in disconnect status or to wrong SSID\n");
                esp_err_t err = esp_wifi_disconnect();
                printf("err: %d\n", err);
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
                err = esp_wifi_connect();
                printf("err: %d\n", err);
                if (err)
                    printf("connect failed error code: %d\n", err);
                else {
                    printf("Station is connected to %s\n", networkStatus.internetSSID);
                    esp_netif_get_ip_info(netif_sta, &ip_info);
                    ESP_LOGI(TAG, "station ssid = %s gw: %x", apInfo.ssid, ip_info.gw.addr);

                }
            }
        }
        else
        {
            printf("Skipping station because busy, busyCnt=%d\n", busyCnt);
            busyCnt--;
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{

    startTach();
    //Initialize NVS
    printf("ESP_WIFI_MAX_CONN_NUM = %d\n", ESP_WIFI_MAX_CONN_NUM);

    printf("message size %d\n", sizeof(messageClientOut));
    uint8_t chipid[6];
    esp_read_mac(chipid, 1);

    newMessage(&messageClientOut, chipid);

    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
    //3c:61:05:0c:8b:01
    //if (chipid[2] == 0x05 && chipid[3] == 0x0c && chipid[4] == 0x8b && chipid[5] == 0x01)  // cOM7

    if (chipid[2] == 0xa8 && chipid[3] == 0x59 && chipid[4] == 0x6a && chipid[5] == 0xa9)
        networkStatus.IAmRoot = 1;
    if (networkStatus.IAmRoot == 0)
        printf("Operating as client\n");
    else
        printf("Operating as server\n");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void *)AF_INET, 5, NULL);

    printf("Network Status internet %d, mesh %d\n", networkStatus.internetRSSI, networkStatus.meshRSSI);

    xTaskCreate(stationScanner, "stationScanner", 4096, NULL, 5, NULL);

    if (networkStatus.IAmRoot == 0)
        xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);

    startHttpServer();
}
