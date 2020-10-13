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
