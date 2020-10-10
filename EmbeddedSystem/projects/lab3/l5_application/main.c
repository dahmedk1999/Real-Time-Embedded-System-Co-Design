#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "delay.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "task.h"
#include <stdio.h>

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
================================================================================*/
// void led_task_part0(void *pvParameters) {
//   /*Set the IOCON MUX function-> 000*/
//   LPC_IOCON->P2_3 &= ~(0b111); // LED Port2 Pin3
//   /*Set Direction (DIR) register bit for the LED*/
//   LPC_GPIO2->DIR |= (1 << 3);

//   while (true) {
//     /* Turn on the LED */
//     LPC_GPIO2->SET |= (1 << 3);
//     vTaskDelay(500);
//     /* Turn off the lED */
//     LPC_GPIO2->CLR |= (1 << 3);
//     vTaskDelay(500);
//   }
// }
/*==================================LED TASK PART2================================
*@brief:    Set LED Blinking
*@para:     Port Pin struct ( port_pin_s->Port, port_pin_s->pin)
*@return:   No return
*@Note:     Using myself driver to config  (DIR, SET, CLR )
=================================================================================*/
// void led_task_part2(void *task_parameter) {
//   /* Type-cast the paramter that was passed from xTaskCreate() */
//   const port_pin_s *led = (port_pin_s *)(task_parameter);
//   gpio1__set_as_output(led->port, led->pin); // set output
//   while (true) {
//     gpio1__set_high(led->port, led->pin);
//     vTaskDelay(100);
//     gpio1__set_low(led->port, led->pin);
//     vTaskDelay(100);
//   }
// }

/*===================================LED TASK PART3=================================
*@brief:    Set LED Blinking--> Trigger by Switch Task
*@para:     Port Pin struct ( port_pin_s->Port, port_pin_s->pin)
*@return:   No return
*@Note:     Using myself driver to config  (DIR, SET, CLR )
            Using SemaphoreBinary to trigger signal
====================================================================================*/
// void led_task_part3(void *task_parameter) {
//   /* Type-cast the paramter that was passed from xTaskCreate() */
//   const port_pin_s *led = (port_pin_s *)(task_parameter);
//   gpio1__set_as_output(led->port, led->pin); // set output

//   while (true) {
//     /* Receive the trigger signal, the process */
//     if (xSemaphoreTake(switch_press_indication, 1000)) {
//       puts("Yes........");
//       /* To do Blink the LED */
//       gpio1__set_high(led->port, led->pin);
//       vTaskDelay(500);
//       gpio1__set_low(led->port, led->pin);
//       vTaskDelay(500);
//     } else {
//       puts("Timeout: No Switch press incation for 1000ms");
//     }
//   }
// }

/*================================SWITCH TASK PART3==================================
*@brief:    Set Switch Task to --> Trigger LED Task
*@para:     Port Pin struct ( port_pin_s->Port, port_pin_s->pin)
*@return:   No return
*@Note:     Using myself driver to config  (DIR, SET, CLR )
            Using SemaphoreBinary to trigger signal
====================================================================================*/
// void switch_task_part3(void *task_parameter) {
//   /* Type-cast the paramter that was passed from xTaskCreate() */
//   port_pin_s *SW = (port_pin_s *)(task_parameter);
//   gpio1__set_as_input(SW->port, SW->pin);

//   while (true) {
//     if (gpio1__get_level(SW->port, SW->pin)) {
//       puts("True");
//       /* Check the trigger signal from switch--> execute LED task */
//       xSemaphoreGive(switch_press_indication);
//     }
//     /**********************************NOTE*****************************************
//     Task should always sleep otherwise they will use 100% CPU
//     This Task sleep also helps avoid spurious semaphore give during switch debounce
//     *******************************************************************************/
//     vTaskDelay(100);
//   }
// }

/*==========================LED TASK PART3-Exrta Credit==========================
*@brief:    Set LED Blinking--> Trigger by Switch Task
*@para:     Port Pin struct ( port_pin_s->Port, port_pin_s->pin)
*@return:   No return
*@Note:     Using myself driver to config  (DIR, SET, CLR )
            Using SemaphoreBinary to trigger signal
=================================================================================*/
// const int port[2] = {1, 2};
// const int pin[4] = {18, 24, 26, 3};
// static port_pin_s array[4] = {{1, 18}, {1, 24}, {1, 26}, {2, 3}};
// uint8_t x = 0;
// uint8_t y = 0;
// uint8_t i = 0;
// void led_task_part4(void *task_parameter) {
//   const port_pin_s *led = (port_pin_s *)(task_parameter);
//   // gpio1__set_as_output(led->port, led->pin);
//   while (true) {

//     // Note: There is no vTaskDelay() here, but we use sleep mechanism while waiting for the binary semaphore
//     if (xSemaphoreTake(switch_press_indication, 1000)) {
//       // TODO: Blink the LED
//       while (true) {
//         /* Not Optimize way; 2 for loop */

//         // for (int x = 0; x <= 1; x++) {
//         //   for (int y = 0; y <= 3; y++) {
//         //     gpio1__set_high(port[x], pin[y]);
//         //     vTaskDelay(100);
//         //     gpio1__set_low(port[x], pin[y]);
//         //     vTaskDelay(100);
//         //   }
//         // }

//         /* Optimize way; 1 for loop */
//         for (i = 0; i <= 4; i++) {
//           gpio1__set(array[i].port, array[i].pin, true);
//           vTaskDelay(200);
//           gpio1__set(array[i].port, array[i].pin, false);
//           vTaskDelay(200);
//         }
//       }
//     } else {
//       puts("Timeout: No switch press indication for 1000ms");
//     }
//   }
// }

/*=============================================LAB Interupt:==================================================*/

/*============================ISR for Switch3-P0_29 (Part 0 + Part2)==============================
*@brief:    Turn OFF the LED P1_18 During ISR
*@para:     No Parameter
*@return:   No return
*@Note:     This function is used for Part 0 and Part 2 (Please check the code below)
=================================================================================================*/
void gpio_ISR(void) {
  fprintf(stderr, "LED P1_18 OFF during ISR \n");
  gpio1__set_high(1, 18); // turn it off
  delay__ms(500);
}

/*================================ISR for Switch2-P0_30 (Part2)==================================
*@brief:    Easter Blinking: Left->Right During ISR
*@para:     No Parameter
*@return:   No return
*@Note:     This function is used for and Part 2 (Please check the code below)
=================================================================================================*/
void gpio_ISR1(void) {
  fprintf(stderr, "Easter Blinking: Left->Right \n");
  static port_pin_s array[4] = {{1, 18}, {1, 24}, {1, 26}, {2, 3}};
  for (int i = 0; i <= 4; i++) {
    gpio1__set(array[i].port, array[i].pin, true);
    delay__ms(200);
    gpio1__set(array[i].port, array[i].pin, false);
    delay__ms(200);
  }
}
/*================================ISR for Switch2-P0_30 (Part1)==================================
*@brief:    Apply the same concept Part0 + using SemaphoreBinary()
*@para:     No Parameter
*@return:   No return
*@Note:     These functions are used for and Part 1 (Please check the code below)
=================================================================================================*/
void gpio_ISR2(void) {
  xSemaphoreGiveFromISR(switch_press_indication, NULL);
  LPC_GPIOINT->IO0IntClr |= (1 << 30);
}

void sleep_on_sem_task(void *p) {
  while (true) {
    if (xSemaphoreTake(switch_press_indication, 1000)) {
      gpio1__set(1, 18, true);
      delay__ms(250);
      gpio1__set(1, 18, false);
      delay__ms(250);
      fprintf(stderr, "Wake up");
    }
  }
}
void main_task_test(void) {
  while (true) {
    puts("Main Task: P1_18 is blinking \n");
    gpio1__set_as_output(1, 18);
    gpio1__set_high(1, 18);
    delay__ms(125);
    gpio1__set_low(1, 18);
    delay__ms(125);
  }
}

int main(void) {

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
  // switch_press_indication = xSemaphoreCreateBinary();
  // static port_pin_s SW1 = {1, 15}; // Switch

  // xTaskCreate(switch_task_part3, "Press Sw1", 2048, &SW1, 2, 0);
  // xTaskCreate(led_task_part4, "Extra Credit", 2048, 0, 2, 0);

  // vTaskDelay(100);

  /************************************ Interrupt part0 *************************************/

  // /* ---> Setup register ( Input --> pull-down ) P0_30 */
  // gpio1__set_as_input(0, 30);
  // LPC_IOCON->P2_3 &= ~(0b10111);

  // /* ---> Setup ISR ( Falling --> NVIC_enable ) P0_30 */
  // LPC_GPIOINT->IO0IntEnF |= (1 << 30);
  // NVIC_EnableIRQ(GPIO_IRQn);

  // /* ---> Use  ISR ( Falling --> NVIC_enable ) P0_30 */
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_ISR, NULL);

  // /* ---> Print something to while loop */
  // while (true) {
  //   puts("Main task is running");
  //   delay__ms(250);
  // }

  /************************ Interrupt part1: Interrupt with Binary Semaphore ***************************/

  // /* ---> Setup register ( Input --> pull-down ) P0_30 */
  // gpio1__set_as_input(0, 30);
  // LPC_IOCON->P2_3 &= ~(0b10111);

  // /* ---> Setup ISR ( Falling --> NVIC_enable ) P0_30 */
  // LPC_GPIOINT->IO0IntEnF |= (1 << 30);
  // NVIC_EnableIRQ(GPIO_IRQn);

  // /* ---> Use  ISR ( Falling --> NVIC_enable ) P0_30 */
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_ISR2, NULL);

  // /* ---> Setup Semaphore */
  // switch_press_indication = xSemaphoreCreateBinary();
  // xTaskCreate(sleep_on_sem_task, "SEM", 2048, NULL, 1, NULL);

  /********************** Interrupt part2:  Interrupt with gpio_isr driver ************************/

  /* =====> Solution 1: Modify the interrupt_vector_table.C <===== */
  /*  This code is currently run by solution 1
   *   Please double check the interrupt_vector_table.c so see the code
   */
  gpio0__attach_interrupt(29, 0, gpio_ISR);  // Fall Edges
  gpio0__attach_interrupt(30, 1, gpio_ISR1); // Rise Edges

  /* =====> Solution 2: Using API inside lpc_peripherals.C <===== */
  /*  Here is the instruction to run the code for as solution 2:
   *  step1: Remove or comment out "gpio0__interrupt_dispatcher" (code line: 101) inside interrupt vector table
   *  step2: Replace it with "lpc_peripheral__interrupt_dispatcher"
   *  step3: uncomment the code line:
   *         "lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, NULL);"
   *
   * @Note: replace the "gpio0__interrupt_dispatcher" to "gpio2__interrupt_dispatcher"
   * ------> if you work with interrupt Port 2
   */
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, NULL); // uncomment here
  // (solution2)

  /* Print something in while loop */
  main_task_test();

  // vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
