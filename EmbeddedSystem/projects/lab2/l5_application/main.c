#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "lpc40xx.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "task.h"
#include <stdio.h>

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

/*=============================Data_Struct_Declaration=====================================*/
typedef struct {
  uint8_t port;
  uint8_t pin;
} port_pin_s;

static SemaphoreHandle_t switch_press_indication; // create semaphore variable

/*==================================LED TASK PART0==============================
*@brief:    Set LED Blinking
*@para:     No Para
*@return:   No return
*@Note:     Manually config IOCON register DIR as Digital Function-Chap7 Table 83
            Manually config GPIO register  ( DIR, SET, CLR)-Chap8 P.148
===========================================================================*/
void led_task_part0(void *pvParameters) {
  /*Set the IOCON MUX function-> 000*/
  LPC_IOCON->P2_3 &= ~(0b111); // LED Port2 Pin3
  /*Set Direction (DIR) register bit for the LED*/
  LPC_GPIO2->DIR |= (1 << 3);

  while (true) {
    /* Turn on the LED */
    LPC_GPIO2->SET |= (1 << 3);
    vTaskDelay(500);
    /* Turn off the lED */
    LPC_GPIO2->CLR |= (1 << 3);
    vTaskDelay(500);
  }
}
/*==================================LED TASK PART2================================
*@brief:    Set LED Blinking
*@para:     Port Pin struct ( port_pin_s->Port, port_pin_s->pin)
*@return:   No return
*@Note:     Using myself driver to config  (DIR, SET, CLR )
=================================================================================*/
void led_task_part2(void *task_parameter) {
  /* Type-cast the paramter that was passed from xTaskCreate() */
  const port_pin_s *led = (port_pin_s *)(task_parameter);
  gpio1__set_as_output(led->port, led->pin); // set output
  while (true) {
    gpio1__set_high(led->port, led->pin);
    vTaskDelay(100);
    gpio1__set_low(led->port, led->pin);
    vTaskDelay(100);
  }
}

/*===================================LED TASK PART3=================================
*@brief:    Set LED Blinking--> Trigger by Switch Task
*@para:     Port Pin struct ( port_pin_s->Port, port_pin_s->pin)
*@return:   No return
*@Note:     Using myself driver to config  (DIR, SET, CLR )
            Using SemaphoreBinary to trigger signal
====================================================================================*/
void led_task_part3(void *task_parameter) {
  /* Type-cast the paramter that was passed from xTaskCreate() */
  const port_pin_s *led = (port_pin_s *)(task_parameter);
  gpio1__set_as_output(led->port, led->pin); // set output

  while (true) {
    /* Receive the trigger signal, the process */
    if (xSemaphoreTake(switch_press_indication, 1000)) {
      puts("Yes........");
      /* To do Blink the LED */
      gpio1__set_high(led->port, led->pin);
      vTaskDelay(500);
      gpio1__set_low(led->port, led->pin);
      vTaskDelay(500);
    } else {
      puts("Timeout: No Switch press incation for 1000ms");
    }
  }
}

/*================================SWITCH TASK PART3==================================
*@brief:    Set Switch Task to --> Trigger LED Task
*@para:     Port Pin struct ( port_pin_s->Port, port_pin_s->pin)
*@return:   No return
*@Note:     Using myself driver to config  (DIR, SET, CLR )
            Using SemaphoreBinary to trigger signal
====================================================================================*/
void switch_task_part3(void *task_parameter) {
  /* Type-cast the paramter that was passed from xTaskCreate() */
  port_pin_s *SW = (port_pin_s *)(task_parameter);
  gpio1__set_as_input(SW->port, SW->pin);

  while (true) {
    if (gpio1__get_level(SW->port, SW->pin)) {
      puts("True");
      /* Check the trigger signal from switch--> execute LED task */
      xSemaphoreGive(switch_press_indication);
    }
    /**********************************NOTE*****************************************
    Task should always sleep otherwise they will use 100% CPU
    This Task sleep also helps avoid spurious semaphore give during switch debounce
    *******************************************************************************/
    vTaskDelay(100);
  }
}

/*==========================LED TASK PART3-Extra Credit==========================
*@brief:    Set LED Blinking--> Trigger by Switch Task
*@para:     Port Pin struct ( port_pin_s->Port, port_pin_s->pin)
*@return:   No return
*@Note:     Using myself driver to config  (DIR, SET, CLR )
            Using SemaphoreBinary to trigger signal
=================================================================================*/
const int port[2] = {1, 2};
const int pin[4] = {18, 24, 26, 3};
static port_pin_s array[4] = {{1, 18}, {1, 24}, {1, 26}, {2, 3}};
uint8_t x = 0;
uint8_t y = 0;
uint8_t i = 0;
void led_task_part4(void *task_parameter) {
  const port_pin_s *led = (port_pin_s *)(task_parameter);
  // gpio1__set_as_output(led->port, led->pin);
  while (true) {

    // Note: There is no vTaskDelay() here, but we use sleep mechanism while waiting for the binary semaphore
    if (xSemaphoreTake(switch_press_indication, 1000)) {
      // TODO: Blink the LED
      while (true) {
        /* Not Optimize way; 2 for loop */

        // for (int x = 0; x <= 1; x++) {
        //   for (int y = 0; y <= 3; y++) {
        //     gpio1__set_high(port[x], pin[y]);
        //     vTaskDelay(100);
        //     gpio1__set_low(port[x], pin[y]);
        //     vTaskDelay(100);
        //   }
        // }

        /* Optimize way; 1 for loop */
        for (i = 0; i <= 4; i++) {
          gpio1__set(array[i].port, array[i].pin, true);
          vTaskDelay(200);
          gpio1__set(array[i].port, array[i].pin, false);
          vTaskDelay(200);
        }
      }
    } else {
      puts("Timeout: No switch press indication for 1000ms");
    }
  }
}

int main(void) {
  // create_blinky_tasks();
  // create_uart_task();
  puts("Starting RTOS");

  /**************************************PART1************************************************
      Blinking the led from function led_task_part0
  ********************************************************************************************/
  // xTaskCreate(led_task_part0, "LED0", 2048, 0, 1, 0);

  /****************************************PART2************************************************
      Create two tasks using led_task function
      Pass each task its own parameter:
      This is static such that these variables will be allocated in RAM and not go out of scope
  **********************************************************************************************/
  // static port_pin_s L1 = {2, 3};
  // static port_pin_s L2 = {1, 18};
  // static port_pin_s L3 = {1, 24};
  // xTaskCreate(led_task_part2, "LED1", 2048, &L1, 1, 0);
  // xTaskCreate(led_task_part2, "LED2", 2048, &L2, 1, 0);
  // xTaskCreate(led_task_part2, "LED3", 2048, &L3, 1, 0);

  /***************************************PART3************************************************
   *  Using Switch task to trigger the Blinking task
   *  Using xSemaphoreCreateBinary();
   *  Hint: Use on-board LED first to get this logic work
   *  After that, you can simply switch these parameter to off board led and switch
   ********************************************************************************************/
  // switch_press_indication = xSemaphoreCreateBinary();
  // static port_pin_s SW1 = {1, 15}; // Switch
  // static port_pin_s L4 = {1, 26};  // LED

  // xTaskCreate(switch_task_part3, "Press Sw1", 2048, &SW1, 2, 0);
  // xTaskCreate(led_task_part3, "LED4", 2048, &L4, 2, 0);

  /**********************************PART3-Extra Credit*****************************************
   *  Using Switch task to trigger the Blinking task
   *  Using xSemaphoreCreateBinary();
   *  Hint: Use on-board LED first to get this logic work
   *  After that, you can simply switch these parameter to off board led and switch
   *********************************************************************************************/
  switch_press_indication = xSemaphoreCreateBinary();
  static port_pin_s SW1 = {1, 15}; // Switch

  xTaskCreate(switch_task_part3, "Press Sw1", 2048, &SW1, 2, 0);
  xTaskCreate(led_task_part4, "Extra Credit", 2048, 0, 2, 0);

  // vTaskDelay(100);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

static void create_blinky_tasks(void) {
  /**
   * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
   * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
   */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes, !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}