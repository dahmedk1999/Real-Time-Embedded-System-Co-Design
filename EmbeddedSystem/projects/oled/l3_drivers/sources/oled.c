#include "oled.h"
/* -------------------------------------------------------------------------- */
void oled_initialization() {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP1);
  /* b) Setup control registers CR0 and CR1 */
  LPC_SSP1->CR0 = 7;        // 8-Bit transfer
  LPC_SSP1->CR1 = (1 << 1); // SSP Control Enable

  /* c) Setup prescalar register to be <= max_clock_mhz-(Input) */
  uint32_t max_clock_mhz = 8 * 1000 * 1000;            // 8-Mhz
  const uint32_t CPU_CLK = clock__get_core_clock_hz(); // 96-MHz
  for (uint8_t divider = 2; divider <= 254; divider += 2) {
    if ((CPU_CLK / divider) <= max_clock_mhz) {
      fprintf(stderr, "LCD_CLK: %d \n", divider);
      break;
    }
    /* Setup PreScale Control[7:0] */
    LPC_SSP1->CPSR = divider;
  }
}

/* -------------------------------------------------------------------------- */

uint8_t ssp1__transfer_byte(uint8_t data_transfer) {
  /* 16-Bits Data Register [15:0] */
  LPC_SSP1->DR = data_transfer;

  /* Status Register-BUSY[4] */
  while (LPC_SSP1->SR & (1 << 4)) {
    ; /* Wait while it is busy(1), else(0) BREAK */
  }
  /* READ 16-Bits Data Register [15:0] */
  return (uint8_t)(LPC_SSP1->DR & 0xFF);
}

/* -------------------------------------------------------------------------- */
/* ------------------------------- Chip Select ------------------------------ */

void oled_CS() {
  gpio1__set_as_output(1, 22);
  gpio1__set_low(1, 22);
}

/* ------------------------------ Chip Deselect ----------------------------- */

void oled_DS() {
  gpio1__set_as_output(1, 22);
  gpio1__set_high(1, 22);
}

/* ------------------------------ Clear Screen ------------------------------ */
void oled_clear() {
  for (int row = 0; row < 8; row++) {
    for (int column = 0; column < 128; column++) {
      bitmap_[row][column] = 0x00;
    }
  }
}
/* ------------------------------- Fill Screen ------------------------------ */
void oled_fill() {
  for (int row = 0; row < 8; row++) {
    for (int column = 0; column < 128; column++) {
      bitmap_[row][column] = 0x00;
    }
  }
}
/* ----------------------- Update Screen with new data ---------------------- */
void oled_update() {
  horizontal_addr_mode();
  for (int row = 0; row < 8; row++) {
    for (int column = 0; column < 128; column++) {
      oled_data();
      ssp1__transfer_byte(bitmap_[row][column]);
    }
  }
}

/* -------------------------------------------------------------------------- */

void turn_on_lcd() {

  config_oled_pin();
  oled_initialization();
  oled_clear();
  oled_CS();
  oled_command();
  panel_init();

  oled_fill();
  oled_update();
  fprintf(stderr, "data in bit_map is %x: ", bitmap_[1][1]);

  oled_clear();
  oled_update(); // after update DC_ should be high. Data is treated as data

  char_C();
  char_M();
  char_P();
  char_E();

  oled_DS();
}

/* -------------------------------------------------------------------------- */

void panel_init(void) {
  /* ----------------------------- Turn off panel ----------------------------- */
  ssp1__transfer_byte(0xAE);

  /* ------------- set display clock divide ratio and ratio value ------------- */
  ssp1__transfer_byte(0xD5);
  ssp1__transfer_byte(0x80);

  /* ---------------------- set multiplex ratio and value --------------------- */
  ssp1__transfer_byte(0xA8);
  ssp1__transfer_byte(0x3F);

  /* ------------------------- Set display offset = 0 ------------------------- */
  ssp1__transfer_byte(0xD3);
  ssp1__transfer_byte(0x00);

  /* --------------------------- Display start line --------------------------- */
  ssp1__transfer_byte(0x40);

  /* --------------------------- charge pump enable --------------------------- */
  ssp1__transfer_byte(0x8D);
  ssp1__transfer_byte(0x14);

  /* ------------------------ Set segman remap 128 to 0 ----------------------- */
  ssp1__transfer_byte(0xA1);

  /* ------------------ Set COM output Scan direction 64 to 0 ----------------- */
  ssp1__transfer_byte(0xC8);

  /* ------------------------- Set pin hardware config ------------------------ */
  ssp1__transfer_byte(0xDA);
  ssp1__transfer_byte(0x12);

  /* ------------------------ Contrast control register ----------------------- */
  ssp1__transfer_byte(0x81);
  ssp1__transfer_byte(0xCF);

  /* -------------------------- Set pre-charge period ------------------------- */
  ssp1__transfer_byte(0xD9);
  ssp1__transfer_byte(0xF1);

  /* -------------------------------- Set Vcomh ------------------------------- */
  ssp1__transfer_byte(0xDB);
  ssp1__transfer_byte(0x40);

  horizontal_addr_mode();

  /* -------------------------- Enable entire display ------------------------- */
  ssp1__transfer_byte(0xA4);

  /* ---------------------- Set  display to normal colors --------------------- */
  ssp1__transfer_byte(0xA6);

  /* ----------------------------- Set display On ----------------------------- */
  ssp1__transfer_byte(0xAF);
}

/* ------------------------ Horizontal Address Mode ----------------------- */

void horizontal_addr_mode() {
  /* -------------------------------------------------------------------------- */
  /* ---------------------------- Set address mode ---------------------------- */

  ssp1__transfer_byte(0x20);
  ssp1__transfer_byte(0x00);

  /* ----------------------------- Set column mode ---------------------------- */
  ssp1__transfer_byte(0x21);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x7F);

  /* ---------------------------- Set page address ---------------------------- */
  ssp1__transfer_byte(0x22);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x07);

  /* -------------------------------------------------------------------------- */
  /* -------------------------------------------------------------------------- */
}

/* -------------------------------------------------------------------------- */

void config_oled_pin() {
  gpio__construct_with_function(0, 7, GPIO__FUNCTION_2);
  gpio1__set_as_output(0, 7);
  gpio__construct_with_function(0, 9, GPIO__FUNCTION_2);
  gpio1__set_as_output(0, 9);
  gpio__construct_with_function(1, 25, GPIO__FUNCITON_0_IO_PIN);
  gpio1__set_as_output(1, 25);
}
/* -------------------------------------------------------------------------- */
void oled_command() { gpio1__set_low(1, 25); }

/* -------------------------------------------------------------------------- */
void oled_data() { gpio1__set_high(1, 25); }

void char_C() {
  ssp1__transfer_byte(0x3E);
  ssp1__transfer_byte(0x41);
  ssp1__transfer_byte(0x41);
  ssp1__transfer_byte(0x41);
  ssp1__transfer_byte(0x22);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
}

void char_M() {
  ssp1__transfer_byte(0x7F);
  ssp1__transfer_byte(0x02);
  ssp1__transfer_byte(0x0C);
  ssp1__transfer_byte(0x02);
  ssp1__transfer_byte(0x7F);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
}

void char_P() {
  ssp1__transfer_byte(0x7F);
  ssp1__transfer_byte(0x09);
  ssp1__transfer_byte(0x09);
  ssp1__transfer_byte(0x09);
  ssp1__transfer_byte(0x06);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
}

void char_E() {
  ssp1__transfer_byte(0x7F);
  ssp1__transfer_byte(0x49);
  ssp1__transfer_byte(0x49);
  ssp1__transfer_byte(0x49);
  ssp1__transfer_byte(0x41);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
}

void char_A() {
  ssp1__transfer_byte(0x7E);
  ssp1__transfer_byte(0x09);
  ssp1__transfer_byte(0x09);
  ssp1__transfer_byte(0x09);
  ssp1__transfer_byte(0x7E);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
  ssp1__transfer_byte(0x00);
}