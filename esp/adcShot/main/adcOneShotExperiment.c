/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "sys/time.h"
#include "sys/types.h"

int timerCount = 0;

//Rick's basic callback for the FreeRTOS timer
//https://www.freertos.org/FreeRTOS-timers-xTimerCreate.html
void vCallbackFunction1(TimerHandle_t xTimer)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    timerCount++;
    printf("timerCount = %d, %lli \n", timerCount, time_us);
}

void app_main(void)
{
    printf("Hello world! 2\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    TickType_t timerTick = 100; // 100 = 1 secound
    void *const timerId  = (void *)0;
    TimerHandle_t timerHandler =
    xTimerCreate("rickTimer",
                 timerTick,
                 pdTRUE,
                 timerId,
                 vCallbackFunction1);

                 if (timerHandler == NULL) {
                    printf("timer not created\n");
                 }

                 xTimerStart(timerHandler, 100);

    int period = 10; // In secounds
    for (int i = 100; i >= 0; i--)
    {
        printf("Restarting in %d seconds...\n", period * i);
        vTaskDelay(period * 1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
