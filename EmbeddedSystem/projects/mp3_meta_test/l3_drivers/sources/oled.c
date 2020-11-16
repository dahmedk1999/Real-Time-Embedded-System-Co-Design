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
  uint32_t SSP1_clock_mhz = 16 * 1000 * 1000;          // 8-Mhz //test15
  const uint32_t CPU_CLK = clock__get_core_clock_hz(); // 96-MHz
  for (uint8_t divider = 2; divider <= 254; divider += 2) {
    if ((CPU_CLK / divider) <= SSP1_clock_mhz) {
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
void oled__transfer_byte(uint8_t data_transfer) {
  /* 16-Bits Data Register [15:0] */
  LPC_SSP1->DR = data_transfer;

  /* Status Register-BUSY[4] */
  while (LPC_SSP1->SR & (1 << 4)) {
    ; /* Wait while it is busy(1), else(0) BREAK */
  }
  /*No need to Read Data Back from MISO */
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
  oled__transfer_byte(0x14);

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
  new_line(0);
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
*@Note:   Please Check The Datasheet of Sequence of Operation
          * Require command_bus (ON)
==============================================================================*/
void horizontal_addr_mode() {

  oled_setC_bus();
  /*  Set address mode  */
  oled__transfer_byte(0x20); // OP Code --> Address range []
  oled__transfer_byte(0x00); // Value = 0x02 (caution!!!)

  /*  Set column mode  */
  oled__transfer_byte(0x21); // OP Code --> Address range []
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x7F);

  /*  Set page address  */
  oled__transfer_byte(0x22); // OP Code --> Address range []
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x07); // ending
}
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void horizontal_scrolling(page_address start_page, page_address stop_page) {
  oled_CS();
  {
    oled_setC_bus();
    // oled__transfer_byte(0x2E); // OP_code Deactivate scrolling
    oled__transfer_byte(0x26); // OP_code Set Funct  scrolling
    oled__transfer_byte(0x00); // dummy byte
    oled__transfer_byte(0x00 | start_page);
    oled__transfer_byte(0x05); // OP_code Frame speed
    oled__transfer_byte(0x00 | stop_page);
    oled__transfer_byte(0x00); // dummy byte
    oled__transfer_byte(0xFF); // dummy byte
    oled__transfer_byte(0x2F); // OP_code Activate scrolling
  }
  oled_DS();
}

/*================================= New Line =================================
*@brief:  Display String In Specific Line ( Pages Addressing Mode )
*@Note:   Please Check The Datasheet Pages37
          * Require command_bus (ON)
          * Page Address range [0xB0 -- 0xB7]
          * ONE colum = EIGHT seg
          *
_________________________________________________________
|Colum0|Colum1|Colum2|Colum3|Colum4|Colum5|Colum6|Colum7|
---------------------------------------------------------
|8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg | --> Page0
|8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg | --> Page1
|8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg | --> Page2
|8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg | --> Page3
|8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg | --> Page4
|8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg | --> Page5
|8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg | --> Page6
|8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg |8-seg | --> Page7
==============================================================================*/
// add parameter cho new line
void new_line(uint8_t line_address) {
  oled_setC_bus();
  /* Page Add [0xB0-0xB7] */
  oled__transfer_byte(0xB0 | line_address);
  /*************************
   * Pages Addressing mode
   * --> Set Colum + SEG <--
   *************************/
  uint8_t start_SEG = 0x00;
  uint8_t start_COLUM = 0x10;
  oled__transfer_byte(start_SEG);
  oled__transfer_byte(start_COLUM);

  oled_setD_bus();
}
void oled_invert(page_address page_num) {
  oled_CS();
  {
    oled_setC_bus();
    oled__transfer_byte(0xB0 | page_num);
    oled__transfer_byte(0xA7);
    oled_setD_bus();
  }
  oled_DS();
}

/*================================== oled print ===============================
*@brief:  Using pointer to Print string
*@para:   *message
          *page number
          *init_or_not
*@Note:   Ready to Call on main.c (all initialization INCLUDED )
          The first time call need to init
          --> So we can print Multi-line(page) with different value
==============================================================================*/
void oled_print(char *message, page_address page_num, multiple_line init_or_not) {

  if (init_or_not) {
    /* Hardware init + Table inti */
    config_oled_pin();
    SPI_oled_initialization();
    char_array_table();

    oled_CS();
    panel_init();
    /* Require This to initial al Pixel */
    oled_clear();
    oled_update();

    /*Select Row [7 <-> 0]*/
    new_line(page_num);

    /* Use Lookup Table to search char and Display */
    display_char(message);
    oled_DS();
  } else {
    oled_CS();
    /*Select Row [7 <-> 0]*/
    new_line(page_num);

    /* Use Lookup Table to search char and Display */
    display_char(message);
    oled_DS();
  }
}

/* -------------------------------------------------------------------------- */
/* --------------------------- LOOK UP char Array --------------------------- */
/* -------------------------------------------------------------------------- */

/* ASCII-128 Slot */
static function_pointer_char char_callback[127];

/*================================ Display Char ==============================
*@brief:  Using pointer to Print string
*@Note:   Base in Ascii value to search the lookup table
==============================================================================*/
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

/**********************************************************
 * Casting to get the ASCII value of the char             *
 * ---> Assign ASCII value to index of Call Back Array    *
 **********************************************************/
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
  char_callback[(int)'P'] = char_P;
  char_callback[(int)'Q'] = char_Q;
  char_callback[(int)'R'] = char_R;
  char_callback[(int)'S'] = char_S;
  char_callback[(int)'T'] = char_T;
  char_callback[(int)'U'] = char_U;
  char_callback[(int)'V'] = char_V;
  char_callback[(int)'W'] = char_W;
  char_callback[(int)'X'] = char_X;
  char_callback[(int)'Y'] = char_Y;
  char_callback[(int)'Z'] = char_Z;
  char_callback[(int)'a'] = char_a;
  char_callback[(int)'b'] = char_b;
  char_callback[(int)'c'] = char_c;
  char_callback[(int)'d'] = char_d;
  char_callback[(int)'e'] = char_e;
  char_callback[(int)'f'] = char_f;
  char_callback[(int)'g'] = char_g;
  char_callback[(int)'h'] = char_h;
  char_callback[(int)'i'] = char_i;
  char_callback[(int)'j'] = char_j;
  char_callback[(int)'k'] = char_k;
  char_callback[(int)'l'] = char_l;
  char_callback[(int)'m'] = char_m;
  char_callback[(int)'n'] = char_n;
  char_callback[(int)'o'] = char_o;
  char_callback[(int)'p'] = char_p;
  char_callback[(int)'q'] = char_q;
  char_callback[(int)'r'] = char_r;
  char_callback[(int)'s'] = char_s;
  char_callback[(int)'t'] = char_t;
  char_callback[(int)'u'] = char_u;
  char_callback[(int)'v'] = char_v;
  char_callback[(int)'w'] = char_w;
  char_callback[(int)'x'] = char_x;
  char_callback[(int)'y'] = char_y;
  char_callback[(int)'z'] = char_z;

  char_callback[(int)'0'] = char_0;
  char_callback[(int)'1'] = char_1;
  char_callback[(int)'2'] = char_2;
  char_callback[(int)'3'] = char_3;
  char_callback[(int)'4'] = char_4;
  char_callback[(int)'5'] = char_5;
  char_callback[(int)'6'] = char_6;
  char_callback[(int)'7'] = char_7;
  char_callback[(int)'8'] = char_8;
  char_callback[(int)'9'] = char_9;

  char_callback[(int)'"'] = char_dquote;
  char_callback[(int)'\''] = char_squote;
  char_callback[(int)','] = char_comma;
  char_callback[(int)'?'] = char_qmark;
  char_callback[(int)'!'] = char_excl;
  char_callback[(int)'@'] = char_at;
  char_callback[(int)'_'] = char_undersc;
  char_callback[(int)'*'] = char_star;
  char_callback[(int)'#'] = char_hash;
  char_callback[(int)'%'] = char_percent;

  char_callback[(int)'&'] = char_amper;
  char_callback[(int)'('] = char_parenthL;
  char_callback[(int)')'] = char_parenthR;
  char_callback[(int)'+'] = char_plus;
  char_callback[(int)'-'] = char_minus;
  char_callback[(int)'/'] = char_div;
  char_callback[(int)':'] = char_colon;
  char_callback[(int)';'] = char_scolon;
  char_callback[(int)'<'] = char_less;
  char_callback[(int)'>'] = char_greater;

  char_callback[(int)'='] = char_equal;
  char_callback[(int)'['] = char_bracketL;
  char_callback[(int)'\\'] = char_backslash;
  char_callback[(int)']'] = char_bracketR;
  char_callback[(int)'^'] = char_caret;
  char_callback[(int)'`'] = char_bquote;
  char_callback[(int)'{'] = char_braceL;
  char_callback[(int)'}'] = char_braceR;
  char_callback[(int)'|'] = char_bar;
  char_callback[(int)'~'] = char_tilde;

  char_callback[(int)' '] = char_space;
  char_callback[(int)'.'] = char_period;
  char_callback[(int)'$'] = char_dollar;
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

void char_Q() {
  oled__transfer_byte(0x3E);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x51);
  oled__transfer_byte(0x21);
  oled__transfer_byte(0x5E);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_R() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x19);
  oled__transfer_byte(0x29);
  oled__transfer_byte(0x46);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_S() {
  oled__transfer_byte(0x26);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x32);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_T() {
  oled__transfer_byte(0x01);
  oled__transfer_byte(0x01);
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x01);
  oled__transfer_byte(0x01);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_U() {
  oled__transfer_byte(0x3F);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x3F);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_V() {
  oled__transfer_byte(0x1F);
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x1F);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_W() {
  oled__transfer_byte(0x3F);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x38);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x3F);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_X() {
  oled__transfer_byte(0x63);
  oled__transfer_byte(0x14);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x14);
  oled__transfer_byte(0x63);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_Y() {
  oled__transfer_byte(0x07);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x70);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x07);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_Z() {
  oled__transfer_byte(0x61);
  oled__transfer_byte(0x51);
  oled__transfer_byte(0x49);
  oled__transfer_byte(0x45);
  oled__transfer_byte(0x43);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_a() {
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x78);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_b() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x38);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_c() {
  oled__transfer_byte(0x38);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_d() {
  oled__transfer_byte(0x38);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_e() {
  oled__transfer_byte(0x38);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x18);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_f() {
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x7E);
  oled__transfer_byte(0x09);
  oled__transfer_byte(0x01);
  oled__transfer_byte(0x02);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_g() {
  oled__transfer_byte(0x18);
  oled__transfer_byte(0xA4);
  oled__transfer_byte(0xA4);
  oled__transfer_byte(0xA4);
  oled__transfer_byte(0x7C);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_h() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x78);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_i() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x7D);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_j() {
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x80);
  oled__transfer_byte(0x80);
  oled__transfer_byte(0x84);
  oled__transfer_byte(0x7D);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_k() {
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x10);
  oled__transfer_byte(0x28);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_l() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x41);
  oled__transfer_byte(0x7F);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_m() {
  oled__transfer_byte(0x7C);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x18);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x78);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_n() {
  oled__transfer_byte(0x7C);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x78);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_o() {
  oled__transfer_byte(0x38);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x38);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_p() {
  oled__transfer_byte(0xFC);
  oled__transfer_byte(0x24);
  oled__transfer_byte(0x24);
  oled__transfer_byte(0x24);
  oled__transfer_byte(0x18);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_q() {
  oled__transfer_byte(0x18);
  oled__transfer_byte(0x24);
  oled__transfer_byte(0x24);
  oled__transfer_byte(0x28);
  oled__transfer_byte(0xFC);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_r() {
  oled__transfer_byte(0x7C);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x08);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_s() {
  oled__transfer_byte(0x48);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_t() {
  oled__transfer_byte(0x04);
  oled__transfer_byte(0x3E);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_u() {
  oled__transfer_byte(0x3C);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x7C);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_v() {
  oled__transfer_byte(0x1C);
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x20);
  oled__transfer_byte(0x1C);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

void char_w() {
  oled__transfer_byte(0x3c);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x30);
  oled__transfer_byte(0x40);
  oled__transfer_byte(0x3C);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_x() {
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x28);
  oled__transfer_byte(0x10);
  oled__transfer_byte(0x28);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_y() {
  oled__transfer_byte(0x9C);
  oled__transfer_byte(0xA0);
  oled__transfer_byte(0xA0);
  oled__transfer_byte(0xA0);
  oled__transfer_byte(0x7C);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_z() {
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x64);
  oled__transfer_byte(0x54);
  oled__transfer_byte(0x4C);
  oled__transfer_byte(0x44);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}

/*Danish's contribution*/
void char_0() {
  oled__transfer_byte(0b00111110);
  oled__transfer_byte(0b01010001);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01000101);
  oled__transfer_byte(0b00111110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_1() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01000010);
  oled__transfer_byte(0b01111111);
  oled__transfer_byte(0b01000000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_2() {
  oled__transfer_byte(0b01000010);
  oled__transfer_byte(0b01100001);
  oled__transfer_byte(0b01010001);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01000110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_3() {
  oled__transfer_byte(0b00100010);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_4() {
  oled__transfer_byte(0b00011000);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b00010010);
  oled__transfer_byte(0b01111111);
  oled__transfer_byte(0b00010000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_5() {
  oled__transfer_byte(0b00100111);
  oled__transfer_byte(0b01000101);
  oled__transfer_byte(0b01000101);
  oled__transfer_byte(0b01000101);
  oled__transfer_byte(0b00111001);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_6() {
  oled__transfer_byte(0b00111100);
  oled__transfer_byte(0b01001010);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b00110000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_7() {
  oled__transfer_byte(0b00000001);
  oled__transfer_byte(0b01110001);
  oled__transfer_byte(0b00001001);
  oled__transfer_byte(0b00000101);
  oled__transfer_byte(0b00000011);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_8() {
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_9() {
  oled__transfer_byte(0b00000110);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b00101001);
  oled__transfer_byte(0b00011110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_dquote() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b00000111);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b00000111);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_squote() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b00000101);
  oled__transfer_byte(0b00000011);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_comma() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b10100000);
  oled__transfer_byte(0b01100000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_qmark() {
  oled__transfer_byte(0b00000010);
  oled__transfer_byte(0b00000001);
  oled__transfer_byte(0b01010001);
  oled__transfer_byte(0b00001001);
  oled__transfer_byte(0b00000110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_excl() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01011111);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_at() {
  oled__transfer_byte(0b00110010);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01111001);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0b00111110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_undersc() {
  oled__transfer_byte(0b10000000);
  oled__transfer_byte(0b10000000);
  oled__transfer_byte(0b10000000);
  oled__transfer_byte(0b10000000);
  oled__transfer_byte(0b10000000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_star() {
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00111110);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_hash() {
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b01111111);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b01111111);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_percent() {
  oled__transfer_byte(0b00100011);
  oled__transfer_byte(0b00010011);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b01100100);
  oled__transfer_byte(0b01100010);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_amper() {
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0b01001001);
  oled__transfer_byte(0b01010101);
  oled__transfer_byte(0b00100010);
  oled__transfer_byte(0b01010000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_parenthL() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b00011100);
  oled__transfer_byte(0b00100010);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_parenthR() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0b00100010);
  oled__transfer_byte(0b00011100);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_plus() {
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00111110);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_minus() {
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_div() {
  oled__transfer_byte(0b00100000);
  oled__transfer_byte(0b00010000);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00000100);
  oled__transfer_byte(0b00000010);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_colon() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_scolon() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01010110);
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_less() {
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b00100010);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_greater() {
  oled__transfer_byte(0b10000010);
  oled__transfer_byte(0b01000100);
  oled__transfer_byte(0b00101000);
  oled__transfer_byte(0b00010000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_equal() {
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0b00010100);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_bracketL() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01111111);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_backslash() {
  oled__transfer_byte(0b00000010);
  oled__transfer_byte(0b00000100);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00010000);
  oled__transfer_byte(0b00100000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_bracketR() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0b01111111);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_caret() {
  oled__transfer_byte(0b00000100);
  oled__transfer_byte(0b00000010);
  oled__transfer_byte(0b00000001);
  oled__transfer_byte(0b00000010);
  oled__transfer_byte(0b00000100);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_bquote() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b00000001);
  oled__transfer_byte(0b00000010);
  oled__transfer_byte(0b00000100);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_braceL() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_braceR() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01000001);
  oled__transfer_byte(0b00110110);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_bar() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01111111);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_tilde() {
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00000100);
  oled__transfer_byte(0b00000100);
  oled__transfer_byte(0b00001000);
  oled__transfer_byte(0b00000100);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_space() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_period() {
  oled__transfer_byte(0x00);
  oled__transfer_byte(0b01100000);
  oled__transfer_byte(0b01100000);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
void char_dollar() {
  oled__transfer_byte(0b00100100);
  oled__transfer_byte(0b00101010);
  oled__transfer_byte(0b01101011);
  oled__transfer_byte(0b00101010);
  oled__transfer_byte(0b00010010);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
  oled__transfer_byte(0x00);
}
