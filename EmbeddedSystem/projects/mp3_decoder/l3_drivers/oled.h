#pragma once

#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "gpio_lab.h"
#include <string.h>
typedef enum {
  page_0,
  page_1,
  page_2,
  page_3,
  page_4,
  page_5,
  page_6,
  page_7,
} page_address;

typedef enum {
  not_init,
  init,
} multiple_line;

/* -------------------------------------------------------------------------- */
/* ------------------------- Declaration + Power on ------------------------- */

/* Oled Pixel for single Page (8)H x (128)W*/
uint8_t bitmap_[8][128];

/* Output [P1_22] ( ON<--(LOW)|(HIGH) -->OFF ) */
void oled_CS();
void oled_DS();

/* SSP1_I/O Function PIN P0_7 | P0_9 | P1_25 */
void config_oled_pin();

/* Command Buss P1_25(LOW) | Data Buss P1_25(HIGH) */
void oled_setC_bus();
void oled_setD_bus();

/* -------------------------------------------------------------------------- */
/* ------------------------ Initialization + Testing ------------------------ */

/* SPI_oled Initial */
void SPI_oled_initialization();

/* Oled (SPI1) Transfer Byte  */
void oled__transfer_byte(uint8_t data_transfer);

/* Initialize the Sequence of OP-CODE for OLED */
void panel_init();

/* Test <-> Turn LCD ON  --> Print ("CMPE") */
void turn_on_lcd();

/* -------------------------------------------------------------------------- */
/* ----------------------- Clear + Fill Update Screen ----------------------- */

/* Set Bit Map with 0x00  */
void oled_clear();

/* Set Bit Map with 0xFF  */
void oled_fill();

/* Update BitMap to Oled */
void oled_update();

/* -------------------------------------------------------------------------- */
/* ------------------------------ Control Usage ----------------------------- */

/* Horizontal Address Mode */
void horizontal_addr_mode();

/* Horizontal Scrolling in given page range */
void horizontal_scrolling(page_address start_page, page_address stop_page);

/* Display String in specific line */
void new_line(uint8_t line_address);

/**/
void oled_print(char *message, uint8_t pages_num, multiple_line init_or_not);

/* -------------------------------------------------------------------------- */
/* --------------------------- LOOK UP char Array --------------------------- */

/* Call Back Array for char */
typedef void (*function_pointer_char)(void);

/*
 * Casting to get the ASCII value of the char
 * ---> Assign ASCII value to index of Call Back Array
 */
void char_array_table();

/* Use Lookup Table to search char and Display */
void display_char(char *string);

/* ----------------------------- Covert to Pixel ---------------------------- */
void char_A();
void char_B();
void char_C();
void char_D();
void char_E();
void char_F();
void char_G();
void char_H();
void char_I();
void char_J();
void char_K();
void char_L();
void char_M();
void char_N();
void char_O();
void char_P();
void char_Q();
void char_R();
void char_S();
void char_T();
void char_U();
void char_V();
void char_W();
void char_X();
void char_Y();
void char_Z();
/*-----------------------------------------------------------*/
void char_a();
void char_b();
void char_c();
void char_d();
void char_e();
void char_f();
void char_g();
void char_h();
void char_i();
void char_j();
void char_k();
void char_l();
void char_m();
void char_n();
void char_o();
void char_p();
void char_q();
void char_r();
void char_s();
void char_t();
void char_u();
void char_v();
void char_w();
void char_x();
void char_y();
void char_z();

/* --------------------------------- Number --------------------------------- */
void char_0();
void char_1();
void char_2();
void char_3();
void char_4();
void char_5();
void char_6();
void char_7();
void char_8();
void char_9();

/* ------------------------------ Special Char ------------------------------ */
void char_dquote();
void char_squote();
void char_comma();
void char_qmark();
void char_excl();
void char_at();
void char_undersc();
void char_star();
void char_hash();
void char_percent();
void char_amper();
void char_parenthL();
void char_parenthR();
void char_plus();
void char_minus();
void char_div();
void char_colon();
void char_scolon();
void char_less();
void char_greater();
void char_equal();
void char_bracketL();
void char_backslash();
void char_bracketR();
void char_caret();
void char_bquote();
void char_braceL();
void char_braceR();
void char_bar();
void char_tilde();
void char_space();
void char_period();
void char_dollar();