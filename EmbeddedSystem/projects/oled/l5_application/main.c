#include "FreeRTOS.h"
#include "adc.h"
#include "board_io.h"
#include "common_macros.h"
#include "delay.h"
#include "gpio.h"

#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"

#include "oled.h"
#include "queue.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "ssp2_lab.h"
#include "task.h"
#include <stdio.h>

#include "string.h"
#include "uart_lab.h"

/* ------------------------------- OLED ------------------------------- */
void oled_task() {
  while (1) {
    turn_on_lcd();
  }
}

/***************************************** MAIN LOOP *********************************************
**************************************************************************************************/
int main(void) {

  /* ----------------------------- Initialization ----------------------------- */
  puts("Starting RTOS\n");
  // sj2_cli__init();

  /* --------------------------------- Part 1 --------------------------------- */
  oled_print("->CMPE146:RTOS<-", page_0, colum_0, init);
  oled_print("-2-", 2, colum_0, 0);
  oled_print("3", 2, colum_7, 0);
  oled_print("->4<-", 4, colum_3, 0);
  oled_print("->5<-", 5, colum_3, 0);
  oled_print("LLLLLL", 6, colum_3, 0);
  oled_print("TTTTTT", 6, colum_3, 0);
  oled_clear_page(6, 6);
  // oled_clear_page(5, 5);

  while (1) {
  }

  // vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
