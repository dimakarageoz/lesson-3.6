#include <stdint.h>
#include <math.h>
#include <driver/ledc.h>
#include <esp_log.h>

#include "servo-motor.h"

uint32_t angle_to_duty(int angle) {
    uint32_t pulse_width_us = SERVO_MIN_PULSE_US + (angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / SERVO_MAX_ANGLE);

    return (pulse_width_us * SERVO_MAX_DUTY) / PWM_PERIOD_US;
}

void setup_pwm(ServoMotorConfig* options) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = options->speedMode,
        .timer_num        = options->timerNum,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 50,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = options->speedMode,
        .channel    = options->channel,
        .timer_sel  = options->timerNum,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = options->servoPin,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&ledc_channel);
}

void set_servo_angle(
    ServoMotorConfig* options,
    int inputAngle,
    int maxAngle
) {
    int angle = inputAngle;

    if (angle < 0) {
        angle = 0;
    } else if (angle >= maxAngle) {
        angle = maxAngle;
    }

    angle = angle_to_duty(angle);

    ledc_set_duty(options->speedMode, options->channel, angle);
    ledc_update_duty(options->speedMode, options->channel);
}