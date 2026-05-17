#include <driver/gpio.h>

#include "encoder.h"

static enum EncoderInterruptTrigger swithCommand = EncoderSwitch;
static enum EncoderInterruptTrigger clockCommand = EncoderClock;

void setupEncoder(EncoderOptions* options) {
    gpio_config_t clk_conf = {
        .pin_bit_mask = (1ULL << options->clkPin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .intr_type = options->clkIntrType != EncodeIntOptionNotDefined
            ? options->clkIntrType
            : GPIO_INTR_NEGEDGE,
    };

    gpio_config(&clk_conf);

    gpio_config_t dir_conf = {
        .pin_bit_mask = (1ULL << options->dirPin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1
    };

    gpio_config(&dir_conf);

    if (options->encoderPowerPin != EncodeIntOptionNotDefined) {
        gpio_config_t out_conf = { 
            .pin_bit_mask = (1ULL << options->encoderPowerPin),
            .mode = GPIO_MODE_OUTPUT
        };
        
        gpio_config(&out_conf);
        gpio_set_level(options->encoderPowerPin, 1);        
    }

    if (options->clkInterruptHandler != EncodeIntOptionNotDefined) {
        gpio_install_isr_service(0);
        gpio_isr_handler_add(options->clkPin, options->clkInterruptHandler, &clockCommand);
    }

    if (options->switchPin != EncodeIntOptionNotDefined && options->switchInterruptHandler != EncodeIntOptionNotDefined) {
        gpio_config_t switch_conf = {
            .pin_bit_mask = (1ULL << options->switchPin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = 1,
            .intr_type = options->switchIntrType != EncodeIntOptionNotDefined
                ? options->switchIntrType
                : GPIO_INTR_DISABLE,
        };

        gpio_config(&switch_conf);
        gpio_isr_handler_add(options->switchPin, options->switchInterruptHandler, &swithCommand);
    }
}

