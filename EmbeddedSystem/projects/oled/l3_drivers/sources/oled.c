#include "oled.h"

/* -------------------------------------------------------------------------- */
/* ------------------------------- PIN CONFIG ------------------------------- */
/* -------------------------------------------------------------------------- */

/* Output [P1_22] ( ON<--(LOW)|(HIGH) -->OFF ) */
void oled_CS() {
  gpio1__set_as_output(1, 22);
  gpio1__set_low(1, 22);
}
void oled_DS() {
  gpio1__set_as_output(1, 22);
  gpio1__set_high(1, 22);
}

/* SSP1_I/O Function PIN P0_7 | P0_9 | P1_25 */
void config_oled_pin() {
  gpio__construct_with_function(0, 7, GPIO__FUNCTION_2);
  gpio1__set_as_output(0, 7);
  gpio__construct_with_function(0, 9, GPIO__FUNCTION_2);
  gpio1__set_as_output(0, 9);
  gpio__construct_with_function(1, 25, GPIO__FUNCITON_0_IO_PIN);
  gpio1__set_as_output(1, 25);
}

/* Command Buss P1_25(LOW) | Data Buss P1_25(HIGH) */
void oled_setC_bus() { gpio1__set_low(1, 25); }
void oled_setD_bus() { gpio1__set_high(1, 25); }

/* -------------------------------------------------------------------------- */
/* ------------------------ Initialization + Testing ------------------------ */
/* -------------------------------------------------------------------------- */
/*===============================SPI_1_OLED Config()===========================
*@brief:  Turn ON SPI peripheral SSP1
*@para:   No para
*@return: No Return
*@Note:   Power ON + 8-Bits Transfer + 8-mHz CLK
==============================================================================*/
void SPI_oled_initialization() {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP1);
  /* b) Setup control registers CR0 and CR1 */
  LPC_SSP1->CR0 = 7;        // 8-Bit transfer
  LPC_SSP1->CR1 = (1 << 1); // SSP Control Enable

  /* c) Setup Prescalar Register to be <= max_clock_mhz-(Input) */
  uint32_t max_clock_mhz = 8 * 1000 * 1000;            // 8-Mhz
  const uint32_t CPU_CLK = clock__get_core_clock_hz(); // 96-MHz
  for (uint8_t divider = 2; divider <= 254; divider += 2) {
    if ((CPU_CLK / divider) <= max_clock_mhz) {
      // fprintf(stderr, "LCD_CLK: %d \n", divider);
      break;
    }
    /* Setup PreScale Control[7:0] */
    LPC_SSP1->CPSR = divider;
  }
}

/*============================oled__transfer_byte()===========================
*@brief:  OLED Byte Transfer (SPI)
*@para:   Data_transfer ( uint8_t )
*@return: Return 8-bits
=============================================================================*/
uint8_t oled__transfer_byte(uint8_t data_transfer) {
  /* 16-Bits Data Register [15:0] */
  LPC_SSP1->DR = data_transfer;

  /* Status Register-BUSY[4] */
  while (LPC_SSP1->SR & (1 << 4)) {
    ; /* Wait while it is busy(1), else(0) BREAK */
  }
  /* READ 16-Bits Data Register [15:0] */
  return (uint8_t)(LPC_SSP1->DR & 0xFF);
}

/*=============================== Panel init()===============================
*@brief:  Initial Oled Panel
*@Note:   Please Check The Datasheet of Sequence of Operation
          * Require command_bus (ON)
=============================================================================*/
void panel_init() {

  oled_setC_bus();
  /*  Turn off panel  */
  oled__transfer_byte(0xAE);

  /*  set display clock divide ratio and ratio value  */
  oled__transfer_byte(0xD5); // OP-Code
  oled__transfer_byte(0x80);

  /*  set multiplex ratio and value  */
  oled__transfer_byte(0xA8); // OP-Code
  oled__transfer_byte(0x3F);

  /*  Set display offset = 0  */
  oled__transfer_byte(0xD3); // OP-Code
  oled__transfer_byte(0x00);

  /*  Display start line  */
  oled__transfer_byte(0x40); // OP-Code

  /*  charge pump enable  */
  oled__transfer_byte(0x8D); // OP-Code
  oled__transfer_byte(0x14); // OP-Code

  /*  Set segment remap 128 to 0  */
  oled__transfer_byte(0xA1);

  /*  Set COM output Scan direction 64 to 0  */
  oled__transfer_byte(0xC8); // OP-Code

  /*  Set pin hardware config  */
  oled__transfer_byte(0xDA); // OP-Code
  oled__transfer_byte(0x12);

  /*  Contrast control register  */
  oled__transfer_byte(0x81); // OP-Code
  oled__transfer_byte(0xCF);

  /*  Set pre-charge period  */
  oled__transfer_byte(0xD9); // OP-Code
  oled__transfer_byte(0xF1);

  /*  Set Vcomh  */
  oled__transfer_byte(0xDB); // OP-Code
  oled__transfer_byte(0x40);

  horizontal_addr_mode();

  /*  Enable entire display  */
  oled__transfer_byte(0xA4); // OP-Code

  /*  Set display to normal colors  */
  oled__transfer_byte(0xA6); // OP-Code

  /*  Set display On  */
  oled__transfer_byte(0xAF); // OP-Code
}

/*=============================== turn_on_lcd ================================
*@brief:  Test <-> Turn LCD ON  --> Print ("CMPE")
*@Note:   -----------> Sequence of OPERATION <-----------
          * SSP1_I/O Function PIN P0_7 | P0_9 | P1_25
          * SPI_oled Initial ( peripheral )
          * CS PIN (ON)
          * Initial all the Pixel with ( Fill + Clear )
          *
=============================================================================*/
void turn_on_lcd() {
  config_oled_pin();
  SPI_oled_initialization();

  oled_CS();

  panel_init();
  /* Require This to initial al Pixel */
  oled_fill();
  oled_update();
  oled_clear();
  oled_update();

  /* Print ("CMPE") */
  char_C();
  char_M();
  char_P();
  char_E();

  oled_DS();
  printf("LCD Should Turn-Successfully\n");
}

/* -------------------------------------------------------------------------- */
/* ----------------------- Clear + Fill Update Screen ----------------------- */
/* -------------------------------------------------------------------------- */
/*=========================Clear + Fill + Update Screen()=======================
*@brief:  Set All BitMap [8]x[128] with 0x00 or 0xFF
*@note:   Clear + Fill  --> SET BitMap
          Update        --> Transfer BitMap to VDRAM
===============================================================================*/
void oled_clear() {
  for (int row = 0; row < 8; row++) {
    for (int column = 0; column < 128; column++) {
      bitmap_[row][column] = 0x00;
    }
  }
}
/* -------------------------------------------------------------------------- */

void oled_fill() {
  for (int row = 0; row < 8; row++) {
    for (int column = 0; column < 128; column++) {
      bitmap_[row][column] = 0xFF;
    }
  }
}
/* -------------------------------------------------------------------------- */
void oled_update() {
  horizontal_addr_mode();
  for (int row = 0; row < 8; row++) {
    for (int column = 0; column < 128; column++) {
      oled_setD_bus();
      oled__transfer_byte(bitmap_[row][column]);
    }
  }
}

/* -------------------------------------------------------------------------- */
/* ------------------------------ Control Usage ----------------------------- */
/* -------------------------------------------------------------------------- */
/*========================= Horizontal Addressing Mode ========================
*@brief:  Data will print horizontal direction
*@Note:
==============================================================================*/

void horizontal_addr_mode() {

  oled_setC_bus();
  /*  Set address mode  */
  oled__transfer_byte(0x20); // OP Code
  oled__transfer_byte(0x00);

  /*  Set column mode  */
  oled__transfer_byte(0x21); // OP Code
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x7F);

  /*  Set page address  */
  oled__transfer_byte(0x22); // OP Code
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x07);
}

/*================================== oled print ===============================
*@brief:  Using pointer to Print string
*@Note:   Ready to Call on main.c (all initialization INCLUDED )
==============================================================================*/
void oled_print(char *message) {

  /* Hardware init + Table inti */
  config_oled_pin();
  SPI_oled_initialization();
  char_array_table();

  oled_CS();
  panel_init();
  /* Require This to initial al Pixel */
  oled_clear();
  oled_update();

  /* Use Lookup Table to search char and Display */
  display_char(message);

  oled_DS();
}

/* -------------------------------------------------------------------------- */
/* --------------------------- LOOK UP char Array --------------------------- */
/* -------------------------------------------------------------------------- */

/* ASCII-128 Slot */
static function_pointer_char char_callback[127];

void display_char(char *string) {
  oled_CS();
  oled_setD_bus();

  for (int i = 0; i < strlen(string); i++) {
    /* Create + assign */
    function_pointer_char lcd_display = char_callback[(int)(string[i])];
    /* Display */
    lcd_display();
  }
}
/*
 * Casting to get the ASCII value of the char
 * ---> Assign ASCII value to index of Call Back Array
 */
void char_array_table() {
  char_callback[(int)'A'] = char_A;
  char_callback[(int)'B'] = char_B;
  char_callback[(int)'C'] = char_C;
  char_callback[(int)'D'] = char_D;
  char_callback[(int)'E'] = char_E;
  char_callback[(int)'F'] = char_F;
  char_callback[(int)'G'] = char_G;
  char_callback[(int)'H'] = char_H;
  char_callback[(int)'I'] = char_I;
  char_callback[(int)'J'] = char_J;
  char_callback[(int)'K'] = char_K;
  char_callback[(int)'L'] = char_L;
  char_callback[(int)'M'] = char_M;
  char_callback[(int)'N'] = char_N;
  char_callback[(int)'O'] = char_O;
  char_callback[(int)'P'] = char_P;
}

void char_A() {
  oled__transfer_byte(0x7E);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x7E);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_B() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x36);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_C() {
  oled__transfer_byte(0x3E);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x22);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_D() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x3E);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_E() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_F() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x01);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_G() {
  oled__transfer_byte(0x3E);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x3A);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_H() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_I() {
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_J() {
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x3F);
  oled__transfer_byte(0x01);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_K() {
  oled__transfer_byte(0x7f);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x14);
  oled__transfer_byte(0x22);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_L() {
  oled__transfer_byte(0x7f);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_M() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x02);
  oled__transfer_byte(0x0C);
  oled__transfer_byte(0x02);
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_N() {
  oled__transfer_byte(0x7f);
  oled__transfer_byte(0x02);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x7f);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_O() {
  oled__transfer_byte(0x3e);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x3e);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_P() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x06);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

// static void char_L() {
//   oled__transfer_byte();
//   oled__transfer_byte();
//   oled__transfer_byte();
//   oled__transfer_byte();
//   oled__transfer_byte();
//   oled__transfer_byte(0x00);
//   oled__transfer_byte(0x00);
//   oled__transfer_byte(0x00);
// }
