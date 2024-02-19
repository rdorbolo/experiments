/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

//https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_oneshot.html#

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
//https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html#
#include "esp_timer.h"

int timerCount          = 0;
int ricksCallbackCounter = 0;

//Rick's basic callback for the FreeRTOS timer
//https://www.freertos.org/FreeRTOS-timers-xTimerCreate.html
void vCallbackFunction1(TimerHandle_t xTimer)
{
    int64_t getTime = esp_timer_get_time();
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    timerCount++;
    printf("timerCount = %d, %lli getTime=%lli, rickCallbackCounter=%d\n", timerCount, time_us, getTime, ricksCallbackCounter);
}

void ricksCallback(void *arg) {
    ricksCallbackCounter++;
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


    //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html#
    printf("creating high resolution timer\n");
    //timer handle
    esp_timer_handle_t timer_handle;
    //timer args
    const esp_timer_create_args_t timerArgs = {
        .callback = ricksCallback,
        .dispatch_method = ESP_TIMER_TASK,
        .arg      = (void *)0,
        .name     = "ricks timer",
        .skip_unhandled_events = false
    };
    //create the timer
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &timer_handle));
    //start the timer
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, (uint64_t)1000));



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
