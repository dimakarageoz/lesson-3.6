#include <driver/ledc.h>

#define SERVO_MIN_PULSE_US     500
#define SERVO_MAX_PULSE_US     2400
#define PWM_PERIOD_US    20000
#define SERVO_LEDC_RES_BITS    13
#define SERVO_MAX_DUTY         ((1 << SERVO_LEDC_RES_BITS) - 1)

#define SERVO_MAX_ANGLE  180

typedef struct {
    uint8_t servoPin;
    ledc_channel_t channel;
    ledc_mode_t speedMode;
    ledc_timer_t timerNum;
    int precisionMode;
} ServoMotorConfig;

void setup_pwm(ServoMotorConfig* options);

void set_servo_angle(ServoMotorConfig* options, int inputAngle, int maxAngle);