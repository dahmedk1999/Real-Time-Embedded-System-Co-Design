#include "FreeRTOS.h"
#include "adc.h"
#include "board_io.h"
#include "common_macros.h"
#include "delay.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "pwm1.h"
#include "queue.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "task.h"
#include <stdio.h>

/************************************PWM Task (Part 0)****************************************
*@brief:    PWM Task with 3 Channel control RBG LED
*@para:     No Parameter
*@return:   No return
*@Note:     We only need to set PWM configuration once, and the HW will drive
            Expected Output:
            -->Will Dynamicly Change Duty Cycle --> Control RED BLUE GREEN Ratio
 ********************************************************************************************/
void pwm_task(void *p) {
  pwm1__init_single_edge(1000);

  /* Enable PWM */
  pin_configure_pwm_channel_as_io_pin(2, 0);
  pin_configure_pwm_channel_as_io_pin(2, 1);
  pin_configure_pwm_channel_as_io_pin(2, 2);

  uint8_t percent = 0;
  while (1) {
    /* Read MR0 and MR1 Value as Requirement */
    uint32_t match0 = LPC_PWM1->MR0 & 0xFFFFFFFF;
    uint32_t match1 = LPC_PWM1->MR1 & 0xFFFFFFFF;
    fprintf(stderr, "MR0: %d MR1: %d \n", match0, match1);
    /* Set Duty Cycle for 3 Channel */
    pwm1__set_duty_cycle(PWM1__2_0, percent);
    pwm1__set_duty_cycle(PWM1__2_1, 100 - percent);
    pwm1__set_duty_cycle(PWM1__2_2, percent / 2);

    if (++percent > 100) {
      percent = 0;
    }

    vTaskDelay(100);
  }
}

/***************************************ADC Task (Part 1)***************************************
*@brief:    adc_task()-Software + adc_taskBurst()-Hardware
*@para:     No Parameter
*@return:   No return
*@Note:     adc_task()        -->Print out RAW Data
            adc_taskBurst()   -->Voltage
 ***********************************************************************************************/
void adc_task(void *p) {
  /* Enable IOPIN + Initialize ADC */
  adc_enable_P1_31();
  adc__initialize();

  while (1) {
    const uint16_t adc_value = adc__get_adc_value(ADC__CHANNEL_5);
    fprintf(stderr, "ADC Result: %X \n", adc_value);
    vTaskDelay(100);
  }
}

void adc_taskBurst(void *p) {
  /* Enable IOPIN + Initialize ADC + Burst Enable */
  adc_enable_P1_31();
  adc__initialize();
  adc__enable_burst_mode(ADC__CHANNEL_5);

  while (1) {
    const uint16_t adc_value = adc__get_adc_value_burst(ADC__CHANNEL_5);
    fprintf(stderr, "ADC Bust Mode Result: %f V \n", ((double)adc_value / 4095) * 3.3);
    vTaskDelay(100);
  }
}

/************************************ADC + PWM (Part 2)******************************************
*@brief:    Using RTOS Queue to Perform Part1 -Part0
*@para:     No Parameter
*@return:   No return
*@Note:     adc_taskBurst2()    --> get RAW_Value/4095 --> xQueueSend()
            pwm_task2()         --> xQueueReceive(..,...,Delay) --> Control Duty Cycle
            Expect Output:      --> Adjust POT ---> RBG change color
 ***********************************************************************************************/
static QueueHandle_t adc_to_pwm_task_queue;

void adc_taskBurst2(void *p) {
  adc_enable_P1_31();
  adc__initialize();
  adc__enable_burst_mode(ADC__CHANNEL_5);

  while (1) {
    const uint16_t adc_value = adc__get_adc_value_burst(ADC__CHANNEL_5);
    xQueueSend(adc_to_pwm_task_queue, &adc_value, 0);
    fprintf(stderr, "Send ADC: %f V\n", ((double)adc_value / 4095) * 3.3);
    vTaskDelay(100);
  }
}
/* =============================================================== */
void pwm_task2(void *p) {
  /* Set Frequency */
  pwm1__init_single_edge(1000);

  /* Enable PWM */
  pin_configure_pwm_channel_as_io_pin(2, 0); // 2 - 0
  pin_configure_pwm_channel_as_io_pin(2, 1);
  pin_configure_pwm_channel_as_io_pin(2, 2);

  int percent = 0;
  uint16_t adc_value = 0;
  while (1) {
    /* Match register */
    uint32_t match0 = LPC_PWM1->MR0 & 0xFFFFFFFF;
    uint32_t match1 = LPC_PWM1->MR1 & 0xFFFFFFFF;
    fprintf(stderr, "MR0: %d MR1: %d \n", match0, match1);
    /* Receive potentiometer value from queue */
    if (xQueueReceive(adc_to_pwm_task_queue, &adc_value, 1000)) {
      fprintf(stderr, "PWM Received Value: %d\n", adc_value);
      percent = (double)adc_value / 4095.0 * 100;
      pwm1__set_duty_cycle(PWM1__2_0, percent);
      pwm1__set_duty_cycle(PWM1__2_1, percent / 2);
      pwm1__set_duty_cycle(PWM1__2_2, 100 - percent);
      // vTaskDelay(100);
    }
  }
}

/***************************************** MAIN LOOP *********************************************
**************************************************************************************************/
int main(void) {

  puts("Starting RTOS");

  /******************************* Part 0 **************************** */
  // xTaskCreate(pwm_task, "pwm Task", 1024, NULL, 1, NULL);

  /******************************* Part 1 **************************** */
  // xTaskCreate(adc_taskBurst, "adc Task", 1024, NULL, 1, NULL);

  /***************************** Part 2 + 3 ************************** */
  adc_to_pwm_task_queue = xQueueCreate(1, sizeof(uint16_t));
  xTaskCreate(adc_taskBurst2, "adc Task", 1024 / sizeof(void *), NULL, 1, NULL);
  xTaskCreate(pwm_task2, "pmw_task", 1024 / sizeof(void *), NULL, 2, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
