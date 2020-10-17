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

#include "queue.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "ssp2_lab.h"
#include "task.h"
#include <stdio.h>

#include "string.h"
#include "uart_lab.h"
/* -------------------------------------------------------------------------- */
/* ------------------------------- Delcaration ------------------------------ */
typedef enum { SwitchOFF, SwicthON } switch_e;

bool SW_Status_P0_29(void) {
  gpio1__set_as_input(0, 29);
  if (gpio1__get_level(0, 29))
    return true;
  else
    return false;
}
static QueueHandle_t sw_queue;
/***************************************Queue Task (Part 1)*********************************
 *@brief: Checking the behavior of Queue Receive vs Queue Send in FreeRTOS
 *@Note:  Check it with different priority between Producer vs Consumer
 ******************************************************************************************/
/* ---------------------------------- SEND ---------------------------------- */
static void producer_task(void *P) {
  int counter = 0;
  while (1) {

    const switch_e sw_value = SW_Status_P0_29();
    printf("P_beforeSEND\n");
    printf("[%d]P_afterSEND: %s\n", counter, xQueueSend(sw_queue, &sw_value, 0) ? "True" : "False");
    counter++;
    vTaskDelay(1000);
  }
}
/* --------------------------------- RECEIVE -------------------------------- */
static void consumer_task(void *P) {
  switch_e sw_value = SwitchOFF;
  int counter = 0;
  while (1) {

    printf("C_beforeRECEIVE\n");
    printf("[%d]RECEIVED: %s --> Value: %d \n", counter,
           xQueueReceive(sw_queue, &sw_value, portMAX_DELAY) ? "True" : "False", sw_value);
    counter++;
  }
}

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

/***************************************** MAIN LOOP *********************************************
**************************************************************************************************/
int main(void) {

  /* ----------------------------- Initialization ----------------------------- */
  puts("Starting RTOS\n");

  /* --------------------------------- Part 1a --------------------------------- */
  /* ----------------------------- Priority: P > C ---------------------------- */
  // sw_queue = xQueueCreate(1, sizeof(switch_e));
  // xTaskCreate(producer_task, "producer", 2048 / sizeof(void *), NULL, 1, NULL);
  // xTaskCreate(consumer_task, "consumer", 2048 / sizeof(void *), NULL, 2, NULL);

  /*******************************Explanation: OUTPUT******************************
  P_beforeSEND                          ==>Begin 1st (P-Task)
  [0]P_afterSEND: True
  C_beforeRECEIVE
  [0]RECEIVED: True --> Value: 0        ==>Finish 1st iteration(C-Task)
  C_beforeRECEIVE                       ==>Loopback (C-Task)->Sleep(Queue_Empty)
  P_beforeSEND                          ==>Begin 2nd (P-Task)
  [1]P_afterSEND: True
  [1]RECEIVED: True --> Value: 1        ==>Finish 2nd iteration(C-Task)
  C_beforeRECEIVE                       ==>Loopback (C-Task)->Sleep(Queue_Empty)
  P_beforeSEND                          ==>Begin 3rd (P-Task)
  [2]P_afterSEND: True
  [2]RECEIVED: True --> Value: 1        ==>Finish 3rd iteration(C-Task)
  C_beforeRECEIVE                       ==>Loopback (C-Task)->Sleep(Queue_Empty)
  P_beforeSEND                          ==>Begin 4th (P-Task)
  [3]P_afterSEND: True
  [3]RECEIVED: True --> Value: 0        ==>Finish 3rd iteration(C-Task)
  C_beforeRECEIVE
  ********************************************************************************/

  /* --------------------------------- Part 1b --------------------------------- */
  /* ----------------------------- Priority: P < C ---------------------------- */
  // sw_queue = xQueueCreate(1, sizeof(switch_e));
  // xTaskCreate(producer_task, "producer", 2048 / sizeof(void *), NULL, 1, NULL);
  // xTaskCreate(consumer_task, "consumer", 2048 / sizeof(void *), NULL, 2, NULL);

  /*******************************Explanation: OUTPUT******************************
   The output is messup due to Following these reason:
   *
   * -> consumer(H) run first --> However, queue is empty --> Sleep
   * --> It wont have a chance to execute anything below
   * ---> xQueueReceive(..,.., portMAX_DELAY) this never return <portMAX_DELAY>
   * ----> Force consumer_task Sleep
   *
   * -> producer(L) run after consumer sleep
   * --> Print before SEND
   * ---> Push Value to Queue
   * ----> xQueueSend(..,.., 0) return TRUE
   *
    C_beforeRECEIVE
    P_beforeSEND
    [0]RECEIVED: True ue
    Value: 0
    C_beforeRECEIVE
    PC_beforeRECEIVE
    [_beforeRECEIVE
    True
    _beforeRECEI[1]RECEIVED: TruTrue
    Value: 0
    C_beforeRECEIVE
    [_beforeRECEIVE
    True
    P_beforeSEND
    [2]RECEIVED: True --> Value: 0
    C1]P_afterSEND: [1]P_afterSEND: True
    P_beforeSEND
    [3]RECEIVED: True --> Value: 0
    C_beforeRECEIVE
    [_beforeRECEIVE
    True
  ********************************************************************************/

  /* --------------------------------- Part 1c --------------------------------- */
  /* ----------------------------- Priority: P = C ---------------------------- */

  // sw_queue = xQueueCreate(1, sizeof(switch_e));

  // xTaskCreate(producer_task, "producer", 2048 / sizeof(void *), NULL, 1, NULL);
  // xTaskCreate(consumer_task, "consumer", 2048 / sizeof(void *), NULL, 1, NULL);

  /*******************************Explanation: OUTPUT******************************
   * Since Two_Task have the same priority
   * --> P_task or C_task will be start randomly
   * ---> The output and sequence of operation will be based ond blocking time
   * ----> In my output, producer run first
   * -----> It will follow the logic or Part 1a
   *********************************************************************************/

  /* --------------------------------- Part 2 --------------------------------- */
  sj2_cli__init();
  xTaskCreate(blink_task, "led0", 2048 / sizeof(void *), NULL, 2, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
