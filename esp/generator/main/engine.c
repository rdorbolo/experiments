#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
//#define INCLUDE_xTaskDelayUntil 1
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/timer.h"
#include <driver/adc.h>
#include "driver/i2c.h"
#include <soc/adc_channel.h>
#include "esp_intr_alloc.h"

#include "freertos/queue.h"
#include "engine.h"

#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define WRITE_BIT I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ    /*!< I2C master read */
#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                 /*!< I2C ack value */
#define NACK_VAL 0x1                /*!< I2C nack value */

static const gpio_num_t i2c_gpio_sda = 21;
static const gpio_num_t i2c_gpio_scl = 19;
static const uint32_t i2c_frequency = 1000000;
static const i2c_port_t i2c_port = I2C_NUM_0;

static const char TAG[] = "engine.c";

char *STATUS_IDLE = "idle";                      // NOLINT
char *STATUS_VOLTS_HIGH = "vHigh";               // NOLINT
char *STATUS_PWM_USER_LIMITITED = "MinPWMLimit"; // NOLINT
char *STATUS_AMPS_HIGH = "aHigh";                // NOLINT
char *STATUS_ADJUSTING = "adjusting";            // NOLINT

uint altCnt = 0;     // NOLINT
uint altCntLast = 0; // NOLINT
uint timerCnt = 0;   // NOLINT

int gpio35Val;
int gpio35ValMin = 10000000;
int gpio35ValAvg = 10000000;
char *status;
int gpio35ValMax = 0;
int gpio35Target = 0;
Stat_t shunt0_data, shunt1_data;
int shunt0Target = 0;
int adcSamples;

#define GPIO2RESET_PWM 512
int gpio2PwmMin = GPIO2RESET_PWM;
int gpio2PwmMax = GPIO2RESET_PWM;
enum State state = reset;

int lastTime = 0;

#define BUFFSIZE 10
const int buffSize = BUFFSIZE;
int adcAvgBuf[BUFFSIZE];

int bufIndex = 0;
bool avgStart = true;
int samples = 0;
int increasePower = 0;

void getVolts(void *parameters)
{
    // TickType_t xLastWakeTime;
    // xLastWakeTime = xTaskGetTickCount();
    int adcAvg;
    while (1)
    {

        TickType_t currentTime = xTaskGetTickCount();
        int avg = 0;
        for (int i = 0; i < 10; i++)
        {
            avg += adc1_get_raw(ADC1_GPIO35_CHANNEL);
        }
        gpio35Val = avg / 10;
        samples++;
        adcSamples++;

        if (avgStart)
        {
            for (int i = 0; i < buffSize; i++)
            {
                adcAvgBuf[i] = gpio35Val;
            }
            avgStart = false;
        }

        adcAvgBuf[bufIndex] = gpio35Val;
        if (bufIndex >= buffSize - 1)
            bufIndex = 0;
        else
        {
            bufIndex++;
        }
        adcAvg = 0;
        for (int i = 0; i < buffSize; i++)
        {
            adcAvg += adcAvgBuf[i];
        }
        adcAvg = adcAvg / buffSize;

        gpio35ValAvg = adcAvg;

        if (adcAvg > gpio35ValMax)
        {
            gpio35ValMax = adcAvg;
        }
        if (adcAvg < gpio35ValMin)
        {
            gpio35ValMin = adcAvg;
        }
        if ((currentTime - lastTime) > 10000 /*100=1sec*/)
        {

            uint64_t timerValue;
            timer_get_counter_value(0, 1, &timerValue);

            ESP_LOGI(TAG, "samples: %d, delta: %d altCnt : %d timerCnt: %d, pinStatus %d timerCnt %llu adcSamples %d gpio35Val/Min/Max %d/%d/%d, shunt0 %d, shunt1 %d ",
                     samples, altCnt - altCntLast, altCnt, timerCnt, gpio_get_level(ALT_GPIO_PIN), timerValue,
                     adcSamples, gpio35Val, gpio35ValMin, gpio35ValMax, shunt0_data.currentSample, shunt1_data.currentSample);
            altCntLast = altCnt;

            lastTime = currentTime;
            samples = 0;
        }

        // vTaskDelay(10 /*950=950ms*/ / portTICK_PERIOD_MS);
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void intFunc(void *params)
{
    gpio_intr_disable(ALT_GPIO_PIN);
    altCnt++;
    // timer_set_counter_value(TIMER_GROUP_0,TIMER_0, 0x00000000ULL);
    timer_start(TIMER_GROUP_0, TIMER_1);
    // timer_enable_intr(TIMER_GROUP_0, TIMER_0);
}

// bool timerHandle( void * params) {
//     TIMERG0.int_clr_timers.t0 = 1;
//     timerCnt++;
//     return false;
// }

void IRAM_ATTR timer_group0_isr1(void *para)
{
    timer_spinlock_take(TIMER_GROUP_0);
    timerCnt++;
    uint32_t timer_intr = timer_group_get_intr_status_in_isr(TIMER_GROUP_0);

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    // if (timer_intr & TIMER_INTR_T0) {
    //     evt.type = TEST_WITHOUT_RELOAD;
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);

    //    timer_counter_value += (uint64_t) (TIMER_INTERVAL0_SEC * TIMER_SCALE);
    //    timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, timer_idx, timer_counter_value);
    //} else if (timer_intr & TIMER_INTR_T1) {
    //    evt.type = TEST_WITH_RELOAD;
    //   timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_1);
    //} else {
    //    evt.type = -1; // not supported even type
    //}

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    // timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
    TIMERG0.int_clr_timers.t0 = 1;
    timer_spinlock_give(TIMER_GROUP_0);
}

#define TIMER_DIVIDER 16                             //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds
#define TIMER_INTERVAL0_SEC (3.4179)                 // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC (.001)                   // sample test interval for the second timer
#define TEST_WITHOUT_RELOAD 0                        // testing will be done without auto reload
#define TEST_WITH_RELOAD 1                           // testing will be done with auto reload

/*
 * A sample structure to pass events
 * from the timer interrupt handler to the main program.
 */
typedef struct
{
    int type; // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

// xQueueHandle timer_queue;

/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
static void inline print_timer_counter(uint64_t counter_value)
{
    printf("Counter: 0x%08x%08x\n", (uint32_t)(counter_value >> 32),
           (uint32_t)(counter_value));
    printf("Time   : %.8f s\n", (double)counter_value / TIMER_SCALE);
}

/*
 * Timer group0 ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 */
void IRAM_ATTR timer_group0_isr(void *para)
{
    timer_spinlock_take(TIMER_GROUP_0);
    timer_group_set_counter_enable_in_isr(TIMER_GROUP_0, TIMER_1, TIMER_PAUSE);
    int timer_idx = (int)para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t timer_intr = timer_group_get_intr_status_in_isr(TIMER_GROUP_0);
    uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(TIMER_GROUP_0, timer_idx);

    /* Prepare basic event data
       that will be then sent back to the main program task */
    timer_event_t evt;
    evt.timer_group = 0;
    evt.timer_idx = timer_idx;
    evt.timer_counter_value = timer_counter_value;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if (timer_intr & TIMER_INTR_T0)
    {
        evt.type = TEST_WITHOUT_RELOAD;
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
        timer_counter_value += (uint64_t)(TIMER_INTERVAL0_SEC * TIMER_SCALE);

        timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, timer_idx, timer_counter_value);
    }
    else if (timer_intr & TIMER_INTR_T1)
    {
        evt.type = TEST_WITH_RELOAD;
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_1);
        // timer_group_set_counter_enable_in_isr(TIMER_GROUP_0, TIMER_1, TIMER_PAUSE);
        gpio_intr_enable(ALT_GPIO_PIN);
    }
    else
    {
        evt.type = -1; // not supported even type
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, timer_idx);

    /* Now just send the event data back to the main program task */
    // xQueueSendFromISR(timer_queue, &evt, NULL);
    timer_spinlock_give(TIMER_GROUP_0);
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
static void example_tg0_timer_init(int timer_idx,
                                   bool auto_reload, double timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);

    uint64_t alarm_value;
    timer_get_alarm_value(TIMER_GROUP_0, timer_idx, &alarm_value);
    printf("alarm value : %llu \n", alarm_value);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
                       (void *)timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    // timer_start(TIMER_GROUP_0, timer_idx);
}

void logSample(Stat_t *shunt_data, int16_t data)
{
    shunt_data->currentSample = (int32_t)data;
    shunt_data->sampleCnt++;
    shunt_data->sampleSum += shunt_data->currentSample;
    if (shunt_data->currentSample > shunt_data->maxValue)
        shunt_data->maxValue = shunt_data->currentSample;
    if (shunt_data->currentSample < shunt_data->minValue)
        shunt_data->minValue = shunt_data->currentSample;
}

void getAmps(void *parameters)
{

    uint8_t chip_addr = 0x48;
    int data_addr = 0x0;
    uint8_t dataH = 0;
    uint8_t dataL = 0;
    int count = 0;
    while (1)
    {
        vTaskDelay(5);

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();

        // Select the Data Register
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, chip_addr << 1U | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x0, ACK_CHECK_EN); // select config data register

        // Read from the data registers
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, chip_addr << 1 | READ_BIT, ACK_CHECK_EN);
        i2c_master_read_byte(cmd, &dataH, ACK_VAL);
        i2c_master_read_byte(cmd, &dataL, NACK_VAL);

        // Command is complete
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);

        i2c_cmd_link_delete(cmd);
        cmd = i2c_cmd_link_create();

        // select and program the Config Register
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, chip_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x1, ACK_CHECK_EN); // select config register

        const uint8_t beginConv = 0b1U << 7;
        uint8_t muxSel;
        uint8_t gain;
        if (count % 2 == 0)
        {
            muxSel = 0b000U << 4; // differential A0 A1
            gain = 0b101U << 1;
        }
        else
        {
            muxSel = 0b011U << 4;
            gain = 0b101U << 1; // differential A2 and A3
        }

        i2c_master_write_byte(cmd, beginConv | muxSel | gain, ACK_CHECK_EN); // channel 1
        i2c_master_write_byte(cmd, 0b11100000, ACK_CHECK_EN);

        // Command is complete
        i2c_master_stop(cmd);

        // Run Command sequence
        ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);

        int16_t data;
        data = ((int16_t)dataH << 8) | ((uint16_t)dataL);
        if (count % 2 == 0)
            logSample(&shunt1_data, -data);
        // shunt1_data.currentSample = -data;
        else
            logSample(&shunt0_data, -data);

        if (ret == ESP_OK)
        {
            /*
            if (count % 2 == 0) //(count % 1000 == 0)
            {
                float k0 = 1.0 / 32767.0 * 256.0 //mV
            ;
            float k1 = k0;
            //                printf("0x%04x 0x%04x  %f  %f mV \n", shunt0_data, shunt1_data, ((float)shunt0_data) * k0, (float)shunt1_data * k1);
            //               printf("%d %d  %f  %f mV \n", shunt0_data, shunt1_data, ((float)shunt0_data) * k0, (float)shunt1_data * k1);
        }
        */
        }
        else if (ret == ESP_ERR_TIMEOUT)
        {
            ESP_LOGW(TAG, "I2C Bus is busy");
        }
        else
        {
            ESP_LOGW(TAG, "I2C Read failed");
        }

        i2c_cmd_link_delete(cmd);

        count++;
    }
}

void updateLoop(void *parameters)
{
    int count = 0;
    int32_t lastTime = 0;

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 2;

    char wasDelayed[] = "was delayed";
    char onTime[] = "on schedule";
    char *time_status;
    char *statusCalc; // used to calc the status

    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        time_status = onTime;
        statusCalc = STATUS_ADJUSTING;

        uint32_t pwm1 = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

        if ((gpio35ValAvg > gpio35Target) || (shunt0_data.currentSample > shunt0Target))
        {
            pwm1++;
            if (gpio35ValAvg > gpio35Target)
            {
                statusCalc = STATUS_VOLTS_HIGH;
            }
            if (shunt0_data.currentSample > shunt0Target)
            {
                statusCalc = STATUS_AMPS_HIGH;
            }
        }

        else // (gpio35ValAvg < gpio35Target)
        {
            increasePower++;
            if (increasePower > 3)
            {
                increasePower = 0;
                pwm1--;
            }
        }

        if (pwm1 <= 1)
            pwm1 = 1;
        if (pwm1 > 512)
            pwm1 = 512;

        if (pwm1 < gpio2PwmMin)
        {
            pwm1 = gpio2PwmMin;
        }
        if (pwm1 == gpio2PwmMin)
        {
            statusCalc = STATUS_PWM_USER_LIMITITED;
        }

        if (pwm1 > gpio2PwmMax)
            pwm1 = gpio2PwmMax;

        if (state == ready || state == running)
        {
            if (gpio35ValAvg < 480)
                state = adcLow; // 10V
            if (gpio35ValAvg > 740)
                state = adcHigh; // 14.8V
        }

        if (state != running)
        {
            pwm1 = GPIO2RESET_PWM;
            statusCalc = STATUS_IDLE;
        }

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

        count++;

        status = statusCalc; // Update the global status message now

        if (count % 12000 == 0)
        {
            int64_t t = esp_timer_get_time();
            int32_t tt = (int32_t)(t / 1000);
            int32_t deltaT = tt - lastTime;
            lastTime = tt;

            printf("\n Engine.c: count= %d, tt=%d deltaTime=%d %s\n", count, tt, deltaT, time_status);
        }
    }
}

void startTach()
{
    ESP_LOGI(TAG, "Starting TACH");

    status = STATUS_IDLE;

    gpio_config_t alt;
    alt.intr_type = GPIO_INTR_POSEDGE;
    alt.mode = GPIO_MODE_DEF_INPUT;
    alt.pin_bit_mask = ((uint64_t)1) << ALT_GPIO_PIN;
    alt.pull_up_en = 0x1;
    alt.pull_down_en = 0x0;

    gpio_config(&alt);
    int altGpioPin = ALT_GPIO_PIN;
    gpio_isr_register(intFunc, (void *)(&altGpioPin), ESP_INTR_FLAG_LEVEL2, NULL);

    // timer_queue = xQueueCreate(10, sizeof(timer_event_t));
    // example_tg0_timer_init(TIMER_0, TEST_WITHOUT_RELOAD, TIMER_INTERVAL0_SEC);
    example_tg0_timer_init(TIMER_1, TEST_WITH_RELOAD, TIMER_INTERVAL1_SEC);

    /*
    gpio_config_t pwmIgnition;
    pwmIgnition.intr_type = GPIO_INTR_DISABLE;
    pwmIgnition.mode = GPIO_MODE_OUTPUT;
    pwmIgnition.pin_bit_mask = ((uint64_t)1)  << 2;
    pwmIgnition.pull_up_en   = 0x0;
    pwmIgnition.pull_down_en = 0x0;
    gpio_config(&pwmIgnition);
    gpio_set_level(2, 0);
    */

    ledc_channel_config_t ledc_conf;

    ledc_conf.gpio_num = 2;
    ledc_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_conf.channel = LEDC_CHANNEL_0;
    ledc_conf.intr_type = LEDC_INTR_DISABLE;
    ledc_conf.timer_sel = LEDC_TIMER_0;
    ledc_conf.duty = 512;
    ledc_conf.hpoint = 0;

    ledc_timer_config_t timer_conf;
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = LEDC_TIMER_9_BIT;
    timer_conf.timer_num = LEDC_CHANNEL_0;
    timer_conf.freq_hz = 100;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;

    int err;
    err = ledc_channel_config(&ledc_conf);
    if (err)
        ESP_LOGE(TAG, "error in ledc channel config");
    err = ledc_timer_config(&timer_conf);
    if (err)
        ESP_LOGE(TAG, "error in ledc timer config");
    err = ledc_set_pin(26, LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err)
        ESP_LOGE(TAG, "error in set pin");

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);

    adc1_config_width(ADC_WIDTH_BIT_10);
    adc1_config_channel_atten(ADC1_GPIO35_CHANNEL, ADC_ATTEN_DB_6);

    // Configure the I2C ADS1115
    i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = i2c_gpio_sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = i2c_gpio_scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = i2c_frequency};
    i2c_param_config(i2c_port, &conf);

    xTaskCreate(getVolts, "getVolts", 2048, NULL, 5, NULL);

    xTaskCreate(getAmps, "getAmps", 2048, NULL, 5, NULL);

    xTaskCreate(updateLoop, "updateLoop", 2048, NULL, 5, NULL);
}