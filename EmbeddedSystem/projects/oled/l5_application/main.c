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

/***************************************CLI Task (Part 2)*********************************
 *@brief: create your own CLI handler: taskcontrol (SUSPEND or RESUME) <Task Name >
 *@Note:  Checkout files
          "cli_handler.h"       -- Declare Handler function
          "sj2_cli.c"           -- Declare Handler struct inside sj2_cli__init()
          "handlers_general.c"  -- Definition Handler function
 *****************************************************************************************/

/* ------------------------------- Blink LED0 ------------------------------- */
static void blink_task(void *P) {
  gpio1__set_as_output(1, 18);
  while (1) {
    gpio1__set_high(1, 18);
    vTaskDelay(250);
    gpio1__set_low(1, 18);
    vTaskDelay(250);
  }
}

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

  /* --------------------------------- Part 2 --------------------------------- */
  // sj2_cli__init();
  // oled_initialization();
  turn_on_lcd();
  while (1) {
  }
  // oled_task();
  // xTaskCreate(blink_task, "led0", 2048 / sizeof(void *), NULL, 2, NULL);

  // vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
