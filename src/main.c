const char *TAG = "GPIO_APP";

#include <stdio.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "macros.h"
#include "pins.h"
#include "encoder.h"
#include "servo-motor.h"

TaskHandle_t encoderClkHandle = NULL;
TaskHandle_t encoderSwitchHandle = NULL;
TaskHandle_t encoderSwitchDelayedHandle = NULL;

static const int MAX_SERVO_STEP = 40;

static void IRAM_ATTR encoder_intr_handler(void *arg);

// EncoderOptions_default
static EncoderOptions encoderOptions = {
    .clkPin = GPIO_ENCODER_CLK_PIN,
    .dirPin = GPIO_ENCODER_DIRECTION_PIN,
    .switchPin = GPIO_ENCODER_SWITCH_PIN,
    .encoderPowerPin = GPIO_ENCODER_POWER,
    .switchIntrType = GPIO_INTR_NEGEDGE,
    .clkInterruptHandler = encoder_intr_handler,
    .switchInterruptHandler = encoder_intr_handler,
    .clkIntrType = EncoderOptions_default.clkIntrType,
};

static ServoMotorConfig servoOptions = {
    .servoPin = GPIO_SERVO_PWM_PIN,
    .channel = LEDC_CHANNEL_0,
    .speedMode = LEDC_LOW_SPEED_MODE,
    .timerNum = LEDC_TIMER_0,
    .precisionMode = 0
};

int servoAngle = 0;

void rotateServoToAngle(int angle) {
    ESP_LOGI(TAG, "ServoAngle %d", angle);

    if (angle == 0 || angle == MAX_SERVO_STEP) {
        gpio_set_level(GPIO_SERVO_SIGNAL_PIN, 1);
    } else {
        gpio_set_level(GPIO_SERVO_SIGNAL_PIN, 0);
    }

    set_servo_angle(&servoOptions, angle * SERVO_MAX_ANGLE / MAX_SERVO_STEP, SERVO_MAX_ANGLE);
}

void encoderClkHandlerTask(void *arg) {
    const TickType_t debounceTicks = pdMS_TO_TICKS(20); // debounce window
    static TickType_t lastTick = 0;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        TickType_t now = xTaskGetTickCount();

        /* If event falls within debounce window, drain any extra notifications and ignore */
        if (lastTick != 0 && (now - lastTick) < debounceTicks) {
            while (ulTaskNotifyTake(pdTRUE, 0) > 0) {
                ; // drain accumulated notifications
            }
            continue;
        }

        lastTick = now;

        if (gpio_get_level(GPIO_ENCODER_CLK_PIN) == 1) {
            continue; // ignore rising edge
        }

        int direction = gpio_get_level(GPIO_ENCODER_DIRECTION_PIN);
        
        const int servoStep = servoOptions.precisionMode == 1 ? 1 : 2;

        if (direction == 1) {
            servoAngle = MAX(servoAngle - servoStep, 0);
        } else if (direction == 0) {
            servoAngle = MIN(servoAngle + servoStep, MAX_SERVO_STEP);
        }

        rotateServoToAngle(servoAngle);
    }
}

void encoderSwitchDelayedTask(void *arg) {
    while (1) {
        xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);

        /* Restart the 1000 ms countdown on each new notification */
        while (xTaskNotifyWait(0, ULONG_MAX, NULL, pdMS_TO_TICKS(1200)) == pdPASS) {
            ; // reset countdown
        }

        if (gpio_get_level(GPIO_ENCODER_SWITCH_PIN) == 0) {
            ESP_LOGI(TAG, "Encoder switch delayed task fired after 1200 ms");

            servoAngle = MAX_SERVO_STEP / 2;
    
            rotateServoToAngle(servoAngle);
        }
    }
}

void encoderSwitchHandlerTask(void *arg) {
    const TickType_t debounceTicks = pdMS_TO_TICKS(50); // debounce window
    static TickType_t lastTick = 0;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        TickType_t now = xTaskGetTickCount();

        if (lastTick != 0 && (now - lastTick) < debounceTicks) {
            while (ulTaskNotifyTake(pdTRUE, 0) > 0) {
                ; // drain accumulated notifications
            }
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(20));

        if (gpio_get_level(GPIO_ENCODER_SWITCH_PIN) != 0) {
            continue; 
        }

        lastTick = now;

        if (encoderSwitchDelayedHandle != NULL) {
            xTaskNotifyGive(encoderSwitchDelayedHandle);
        }
   
        servoOptions.precisionMode = servoOptions.precisionMode == 1 ? 0 : 1;

        ESP_LOGI(TAG, "New precisionMode: %s", servoOptions.precisionMode == 1 ? "HIGH" : "LOW");
    }
}

static void IRAM_ATTR encoder_intr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    enum EncoderInterruptTrigger trigger = *((enum EncoderInterruptTrigger*) arg);

    if (trigger == EncoderClock) {
        vTaskNotifyGiveFromISR(encoderClkHandle, &xHigherPriorityTaskWoken);
    } else if (trigger == EncoderSwitch) {
        vTaskNotifyGiveFromISR(encoderSwitchHandle, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void app_main() {
    xTaskCreate(encoderClkHandlerTask, "encoderClkHandlerTask", 4 * 2048, NULL, 2, &encoderClkHandle);
    xTaskCreate(encoderSwitchHandlerTask, "encoderSwitchHandlerTask", 4 * 2048, NULL, 2, &encoderSwitchHandle);
    xTaskCreate(encoderSwitchDelayedTask, "encoderSwitchDelayedTask", 4 * 1024, NULL, 2, &encoderSwitchDelayedHandle);

    setupEncoder(&encoderOptions);
    setup_pwm(&servoOptions);

    gpio_config_t signal_cfg = {
        .pin_bit_mask = 1ULL << GPIO_SERVO_SIGNAL_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&signal_cfg);

    gpio_set_level(GPIO_SERVO_SIGNAL_PIN, 1);
}