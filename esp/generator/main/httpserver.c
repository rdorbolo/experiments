#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_intr_alloc.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#include "esp_netif.h"
#include <lwip/netdb.h>

#include "esp_spiffs.h"

#include "freertos/queue.h"
#include "httpserver.h"
#include "engine.h"

static const char *TAG = "HTTPSERVER";

void startHttpServer()
{
    ESP_LOGI(TAG, "Starting httpserver on Port %d", HTTPSERVER_PORT);

    ESP_LOGI(TAG, "Initializing SPIFFS");

    extern int altCnt;       // from engine.c
    extern char * status;    // from engine.c
    extern int gpio35Val;    // from engine.c
    extern int gpio35ValMin; // from engine.c
    extern int gpio35ValMax; // from engine.c
    extern int gpio35Target; // from engine.c
    extern int gpio2PwmMin;
    extern int gpio2PwmMax;
    extern enum State state;
    extern int busyCnt; // softap_example_main.c
    extern Stat_t shunt0_data, shunt1_data;
    extern int shunt0Target;


    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    ESP_LOGI(TAG, "writting hello.txt");

    // Open for writing hello.txt

    FILE *fpWr = fopen("/spiffs/hello.txt", "w");
    if (fpWr == NULL)
    {
        ESP_LOGE(TAG, "Failed to open hello.txt");
        return;
    }
    char strWr[] = "this is a test";
    fwrite(strWr, 1, sizeof(strWr), fpWr);
    fclose(fpWr);

    FILE *fp = fopen("/spiffs/hello.txt", "r");
    if (fp == NULL)
    {
        ESP_LOGE(TAG, "Failed to open hello.txt");
        return;
    }
    char str[64];
    size_t len;
    len = fread(str, 1, sizeof(str), fp);
    fclose(fp);
    str[len] = 0;
    // Display the read contents from the file
    ESP_LOGI(TAG, "Read from hello.txt: %s", str);

    struct sockaddr_in dest_addr;
    int ip_protocol = 0;

    struct sockaddr_in *dest_addr_ip4 = &dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY); // from Any address / interface
    dest_addr_ip4->sin_family = AF_INET;                // IPV4
    dest_addr_ip4->sin_port = htons(HTTPSERVER_PORT);   // Listen Port
    ip_protocol = IPPROTO_IP;                           // IP

    while (1)
    {
        ESP_LOGI(TAG, "Creating a listen Socket");

        int listen_sock = socket(AF_INET, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            //vTaskDelete(NULL);
            return;
        }

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            goto CLEAN_UP;
        }
        ESP_LOGI(TAG, "Socket bound, port %d", HTTPSERVER_PORT);

        err = listen(listen_sock, 1);
        if (err != 0)
        {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            goto CLEAN_UP;
        }

        char messageIn[200];
        char messageBody[400];
        bool keepAlive = false;
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        
        uint32_t addr_len = sizeof(source_addr);
        int sock;
        struct timeval to;
        u16_t port;
        u32_t addr;
        while (1)
        {
            if (busyCnt < 3)
                busyCnt++;

            if (keepAlive)
            {
                ESP_LOGI(TAG, "Socket KeepAlive");
            }
            else
            {
                ESP_LOGI(TAG, "Socket listening");

                sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
                if (sock < 0)
                {
                    ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                    break;
                }

                to.tv_sec = 5;
                to.tv_usec = 0;
                if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) < 0)
                {
                    ESP_LOGE(TAG, "... failed to set socket receiving timeout");
                }

                port = ((struct sockaddr_in *)&source_addr)->sin_port;
                addr = ((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;

                ESP_LOGI(TAG, "Socket accepted ip address: %u.%u.%u.%u, port: %d", addr & 0x0ff, (0x0ff00 & addr) >> 8, (0x0ff0000 & addr) >> 16, addr >> 24, port);
            }
            //do_retransmit(sock);
            char lastBytes[4];
            char headerLine[100];
            int headerLineIndex = 0;
            bool requestLine = false;
            bool putLine = false;
            bool getReq = false;
            bool putReq = false;
            bool contentLengthLine = false;
            int contentLength = -1;
            bool httpHeader = true;
            int bodyLen = 0;
#define MAXURL 100
            char url[MAXURL];
            char filename[MAXURL + 10];
            FILE *fp = NULL;
            keepAlive = false;
            while (1)
            {
                int len;
                //len = recv(sock, &messageIn, sizeof(messageIn)-1, 0);
                len = recv(sock, &messageIn, 1, 0);
                //gpio_set_level(2, 1);

                if (len < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
                    break;
                }
                else if (len == 0)
                {
                    ESP_LOGW(TAG, "Connection closed");
                    break;
                }
                else
                {
                    if (httpHeader)
                    {
                        if (headerLineIndex < (sizeof(headerLine) - 1) && ((messageIn[0] != '\n') || messageIn[0] != '\r'))
                        {
                            headerLine[headerLineIndex] = messageIn[0];
                            headerLineIndex++;
                        }

                        if (lastBytes[3] == 'G' && lastBytes[2] == 'E' && lastBytes[1] == 'T' && lastBytes[0] == ' ' && headerLineIndex == 5)
                        {
                            //printf("Get Token next\n");
                            requestLine = true;
                            getReq = true;
                        }
                        if (lastBytes[3] == 'P' && lastBytes[2] == 'U' && lastBytes[1] == 'T' && lastBytes[0] == ' ' && headerLineIndex == 5)
                        {
                            //printf("Put Token next\n");

                            putLine = true;
                            putReq = true;
                        }

                        // Content-Length header line
                        if (lastBytes[3] == 'C' && lastBytes[2] == 'o' && lastBytes[1] == 'n' && lastBytes[0] == 't' && headerLineIndex == 5 && putReq)
                        {
                            printf("\nContent Length Header %d\n", headerLineIndex);
                            contentLengthLine = true;
                        }

                        if ((messageIn[0] == '\r') && (requestLine == true || putLine == true))
                        {
                            headerLine[headerLineIndex] = 0;
                            //printf("\n\nheaderLine ->%s\n\n", headerLine);
                            char *start;
                            char *end;
                            start = strstr(headerLine, "/");
                            end = strstr(start, " ");
                            *end = 0;
                            if (start != NULL && end != NULL)
                            {
                                start++;
                                strncpy(url, start, sizeof(url));
                                if (strncmp(url, "", MAXURL - 1) == 0)
                                    strcpy(url, "index.html");

                                filename[0] = 0;
                                strcat(filename, "/spiffs/");
                                strncat(filename, url, MAXURL - 1);

                                printf("URL ->\"%s\" filename=%s\n", url, filename);
                            }
                            //printf("\n\nheaderLine %p start %p end %p \n\n", headerLine, start, end);

                            requestLine = false;
                            putLine = false;
                        }

                        if ((messageIn[0] == '\r') && (contentLengthLine == true))
                        {
                            headerLine[headerLineIndex] = 0;
                            sscanf(headerLine, "Content-Length: %d", &contentLength);
                            printf("\n content Lenght : %d\n", contentLength);

                            contentLengthLine = false;
                        }

                        if (messageIn[0] == '\n')
                        {
                            headerLineIndex = 0;
                        }

                        //                printf("\n %i, %u %c \n", headerLineIndex, (unsigned)messageIn[0], messageIn[0]);

                        lastBytes[3] = lastBytes[2];
                        lastBytes[2] = lastBytes[1];
                        lastBytes[1] = lastBytes[0];
                        lastBytes[0] = messageIn[0];

                        messageIn[len] = 0;
                        //printf("%s", messageIn);

                        if (lastBytes[3] == '\r' && lastBytes[2] == '\n' && lastBytes[1] == '\r' && lastBytes[0] == '\n' && httpHeader)
                        {
                            httpHeader = false;
                            //printf("--- End of Request Headers--- \n");
                            int bytes;

                            if (getReq)
                            {
                                if (strncmp(url, "data", 4) == 0)
                                {

                                    url[MAXURL - 1] = 0;
                                    int pwm1 = -1;
                                    int pwm2 = -2;
                                    int adcTarget = -3;
                                    int pwm1Min = -4;
                                    int pwm1Max = -5;
                                    int stateIn = -6;
                                    int shunt0TargetIn = -70000;
                                    int shunt1TargetIn = -80000;
                                    sscanf(url, "data/pwm=%d,%d,%d,%d,%d,%d,%d,%d", &pwm1, &pwm2, &adcTarget, &pwm1Min, &pwm1Max, &stateIn, &shunt0TargetIn, &shunt1TargetIn);
                                    printf("url:%s\n", url);
                                    printf("pwm1=%d pwm2=%d\n", pwm1, pwm2);

                                    if (pwm1 >= 0)
                                    {
                                        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm1);
                                        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                                    }

                                    if (pwm1Min >= 0)
                                        gpio2PwmMin = pwm1Min;
                                    if (pwm1Max >= 0)
                                        gpio2PwmMax = pwm1Max;

                                    if (adcTarget >= 0)
                                    {
                                        gpio35Target = adcTarget;
                                    }

                                    if (shunt0TargetIn > -1000) {
                                        shunt0Target = shunt0TargetIn;
                                    }

                                    if (stateIn == ready)
                                        state = ready;
                                    else if (state == ready && stateIn == running)
                                        state = running;
                                    else if (state == running && stateIn == stopped)
                                        state = stopped;
                                    else if (state == stopped && stateIn == ready)
                                        state = ready;
                                    else if (state == adcHigh && stateIn == ready)
                                        state = ready;
                                    else if (state == adcLow && stateIn == ready)
                                        state = ready;
                                    else if (stateIn == -6)
                                        state = state;
                                    else if (stateIn == 0)
                                        state = state;
                                    else
                                        state = linkBad;

                                    int bytesJson;
                                    keepAlive = true;

                                    TickType_t ts = xTaskGetTickCount();
                                    pwm1 = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                                    const char *json = " { "
                                                       " \"ts\":%d, "
                                                       " \"altCnt\": %d, "
                                                       " \"status\": \"%s\", "
                                                       " \"gpio35Val\": %d, "
                                                       " \"gpio35ValMin\": %d, "
                                                       " \"gpio35ValMax\": %d, "
                                                       " \"state\": %d, "
                                                       " \"pwm1\" : %d, "
                                                       " \"shunt0\": %d, "
                                                       " \"shunt0Cnt\": %d, "
                                                       " \"shunt0Sum\": %d, "
                                                       " \"shunt0Min\": %d, "
                                                       " \"shunt0Max\": %d, "
                                                       " \"shunt1\" : %d "
                                                       " } ";

                                    bytesJson = snprintf(messageBody, sizeof(messageBody), json,
                                                         ts, 
                                                         altCnt, 
                                                         status, 
                                                         gpio35Val, 
                                                         gpio35ValMin, 
                                                         gpio35ValMax, 
                                                         state, 
                                                         pwm1, 
                                                         shunt0_data.currentSample, shunt0_data.sampleCnt, shunt0_data.sampleSum, shunt0_data.minValue, shunt0_data.maxValue,
                                                         shunt1_data.currentSample);


                                    shunt0_data.sampleCnt = 0;
                                    shunt0_data.sampleSum = 0;
                                    shunt0_data.maxValue  = INT32_MIN;
                                    shunt0_data.minValue  = INT32_MAX;

                                    bytes = snprintf(messageIn, sizeof(messageIn), "HTTP/1.1 200 OK\r\n");
                                    send(sock, messageIn, bytes, 0);
                                    bytes = snprintf(messageIn, sizeof(messageIn), "Connection: keep-alive\r\n");
                                    send(sock, messageIn, bytes, 0);

                                    bytes = snprintf(messageIn, sizeof(messageIn), "Content-Type: application/json\r\n");
                                    send(sock, messageIn, bytes, 0);
                                    bytes = snprintf(messageIn, sizeof(messageIn), "Content-Length: %d\r\n", bytesJson);
                                    send(sock, messageIn, bytes, 0);

                                    bytes = snprintf(messageIn, sizeof(messageIn), "\r\n");
                                    send(sock, messageIn, bytes, 0);

                                    gpio35ValMin = 9999999;
                                    gpio35ValMax = 0;

                                    send(sock, messageBody, bytesJson, 0);
                                    //send(sock, "                                                                                                                          ", contentLength - bytesJson, 0);
                                    break;
                                }
                                else
                                {
                                    fp = fopen(filename, "r");
                                    if (fp == NULL)
                                    {
                                        ESP_LOGI(TAG, "Failed to open %s\n", filename);
                                        bytes = snprintf(messageIn, sizeof(messageIn), "HTTP/1.1 404 Not Found\r\n");
                                        send(sock, messageIn, bytes, 0);
                                        bytes = snprintf(messageIn, sizeof(messageIn), "\r\n");
                                        send(sock, messageIn, bytes, 0);
                                        break;
                                    }
                                    else
                                    {
                                        bytes = snprintf(messageIn, sizeof(messageIn), "HTTP/1.1 200 OK\r\n");
                                        send(sock, messageIn, bytes, 0);

                                        const char *suffix = strrchr(url, '.');

                                        if (suffix == NULL)
                                        {
                                            bytes = snprintf(messageIn, sizeof(messageIn), "Content-Type: text/plain\r\n");
                                            send(sock, messageIn, bytes, 0);
                                        }
                                        else
                                        {
                                            suffix++;
                                            printf("suffix = %s\n", suffix);

                                            if (strcmp("html", suffix) == 0)
                                            {
                                                bytes = snprintf(messageIn, sizeof(messageIn), "Content-Type: text/html\r\n");
                                                send(sock, messageIn, bytes, 0);
                                            }
                                            else if (strcmp("jpg", suffix) == 0)
                                            {
                                                bytes = snprintf(messageIn, sizeof(messageIn), "Content-Type: image/jpeg\r\n");
                                                send(sock, messageIn, bytes, 0);
                                            }
                                            else if (strcmp("css", suffix) == 0)
                                            {
                                                bytes = snprintf(messageIn, sizeof(messageIn), "Content-Type: text/css\r\n");
                                                send(sock, messageIn, bytes, 0);
                                            }
                                            else if (strcmp("js", suffix) == 0)
                                            {
                                                bytes = snprintf(messageIn, sizeof(messageIn), "Content-Type: text/javascript\r\n");
                                                send(sock, messageIn, bytes, 0);
                                            }
                                        }

                                        bytes = snprintf(messageIn, sizeof(messageIn), "\r\n");
                                        send(sock, messageIn, bytes, 0);

                                        //bytes = snprintf(messageIn, sizeof(messageIn), "rick was here!");
                                        while (1)
                                        {
                                            len = fread(messageIn, 1, sizeof(messageIn), fp);
                                            if (!len)
                                                break;
                                            send(sock, messageIn, len, 0);
                                        }
                                        break;
                                    }
                                }
                            }
                            if (putReq)
                            {
                                bytes = snprintf(messageIn, sizeof(messageIn), "HTTP/1.1 100 continue\r\n");
                                send(sock, messageIn, bytes, 0);
                                bytes = snprintf(messageIn, sizeof(messageIn), "HTTP/1.1 200 OK\r\n");
                                send(sock, messageIn, bytes, 0);

                                //bytes = snprintf(messageIn, sizeof(messageIn), "Content-Type: text/plain\r\n");
                                //send(sock, messageIn, bytes, 0);

                                bytes = snprintf(messageIn, sizeof(messageIn), "\r\n");
                                send(sock, messageIn, bytes, 0);

                                // Open for URL for writting

                                fp = fopen(filename, "w");
                                if (fp == NULL)
                                {
                                    ESP_LOGE(TAG, "Failed to open %s\n", filename);
                                    //return;
                                }
                            }
                        }
                    }
                    else // (!httpHeader)
                    {
                        bodyLen += len;
                        if (fp)
                            fwrite(messageIn, 1, len, fp);
                        if (bodyLen >= contentLength)
                            break;
                    }
                } // end of if Valid Data received from socket
            }     //end of Rx while loop
            getReq = false;
            putReq = false;

            //printf("BodyLen = %d\n", bodyLen);
            //gpio_set_level(2, 0);
            if (fp)
                fclose(fp);

            if (!keepAlive)
            {
                shutdown(sock, 0);
                close(sock);
            }
        }

    CLEAN_UP:
        ESP_LOGE(TAG, "CLOSED HTTP LISTEN SOCKET!");
        close(listen_sock);
        ESP_LOGE(TAG, "Waiting here for 60 seconds .. just so it shows in log");
        vTaskDelay(6000);
    }

    vTaskDelete(NULL);
}
