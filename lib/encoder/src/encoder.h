#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define EncodeIntOptionNotDefined -1

enum EncoderInterruptTrigger {
    EncoderClock = 0,
    EncoderSwitch = 1
};

typedef struct {
    int clkPin;
    int dirPin;
    int switchPin;
    int encoderPowerPin;
    gpio_int_type_t clkIntrType;
    gpio_int_type_t switchIntrType;
    void (*clkInterruptHandler)(void *arg);
    void (*switchInterruptHandler)(void *arg);
} EncoderOptions;

static const EncoderOptions EncoderOptions_default = { 
    EncodeIntOptionNotDefined, 
    EncodeIntOptionNotDefined, 
    EncodeIntOptionNotDefined, 
    EncodeIntOptionNotDefined, 
    EncodeIntOptionNotDefined, 
    EncodeIntOptionNotDefined, 
    NULL, 
    NULL
};

void setupEncoder(EncoderOptions* options);