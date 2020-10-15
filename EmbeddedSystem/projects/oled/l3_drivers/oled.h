#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "gpio_lab.h"

uint8_t bitmap_[8][128];

/* */
void oled_initialization();

/* */
uint8_t ssp1__transfer_byte(uint8_t data_transfer);

/* P1_22 --> LOW */
void oled_CS();

/* P1_22 --> HIGH */
void oled_DS();

/* */
void turn_on_lcd();

/* */
void panel_init(void);

/* */
void horizontal_addr_mode();

/* */
void oled_clear();

/* */
void oled_fill();

/* */
void oled_update();

/* */
void config_oled_pin();

/* */
void oled_command();

/* */
void oled_data();

void char_C();
void char_M();
void char_P();
void char_E();
void char_A();