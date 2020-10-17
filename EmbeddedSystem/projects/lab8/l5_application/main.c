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

#include "acceleration.h"
#include "string.h"
#include "uart_lab.h"

#include "ff.h"
/* -------------------------------------------------------------------------- */
/* ------------------------------- Delcaration ------------------------------ */

static QueueHandle_t sensor_queue;

/* -------------------------- Write file to SD card ------------------------- */
void write_file_using_fatfs_spi(void) {
  const char *filename = "ouput.txt";
  FIL file; // File handle
  UINT bytes_written = 0;
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_CREATE_ALWAYS));
  if (FR_OK == result) {
    char string[64];
    sprintf(string, "Value,%i\n", 123);
    if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
    } else {
      printf("ERROR: Failed to write data to file\n");
    }
    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }

  // char string2[64];
  // f_read(&file, &string2, strlen(string2), &bytes_written);
  // puts("after\n");
  // for (int i = 0; i < strlen(string2); i++) {
  //   fprintf(stderr, "%c", string2[i]);
  // }
}

/***************************************Queue Task (Part 1)*********************************
 *@brief: Checking the behavior of Queue Receive vs Queue Send in FreeRTOS
 *@Note:  Check it with different priority between Producer vs Consumer
 ******************************************************************************************/
typedef struct {
  acceleration__axis_data_s sample[100];
  double sum_y;
  double avg_y;
} data;

/* ---------------------------------- SEND ---------------------------------- */
static void producer_task(void *P) {
  /*Initialize sensor*/
  fprintf(stderr, "Sensor Status: %s\n", acceleration__init() ? "Ready" : "Not Ready");
  data data1;
  while (1) {

    for (int i = 0; i < 100; i++) {
      data1.sample[i] = acceleration__get_data();
      data1.sum_y += data1.sample[i].y;
    }
    data1.avg_y = data1.sum_y / 100;
    printf("EnQueue: %s\n", xQueueSend(sensor_queue, &data1.avg_y, 0) ? "1" : "0");
    vTaskDelay(1000);
  }
}
/* --------------------------------- RECEIVE -------------------------------- */
static void consumer_task(void *P) {
  const char *filename = "file.txt";
  FIL file; // File handle
  UINT bytes_written = 0;
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_CREATE_ALWAYS));
  double receive;
  while (1) {
    if (xQueueReceive(sensor_queue, &receive, portMAX_DELAY)) {
      if (FR_OK == result) {
        static char string[64];
        sprintf(string, "Avg: %f\n", receive);
        if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {

          /* Save progress*/
          f_sync(&file);
          vTaskDelay(10);
        } else {
          printf("ERROR: Failed to write data to file\n");
        }
      } else {
        printf("ERROR: Failed to open: %s\n", filename);
      }
    }
    printf("Avg: %f\n", receive);
  }
}

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
  // write_file_using_fatfs_spi();
  /* ----------------------------- Priority: P = C ---------------------------- */

  sensor_queue = xQueueCreate(1, sizeof(double));

  xTaskCreate(producer_task, "producer", 2048 / sizeof(void *), NULL, 2, NULL);
  xTaskCreate(consumer_task, "consumer", 2048 / sizeof(void *), NULL, 2, NULL);

  /*******************************Explanation: OUTPUT******************************
   * Since Two_Task have the same priority
   * --> P_task or C_task will be start randomly
   * ---> The output and sequence of operation will be based ond blocking time
   * ----> In my output, producer run first
   * -----> It will follow the logic or Part 1a
   *********************************************************************************/

  /* --------------------------------- Part 2 --------------------------------- */
  // sj2_cli__init();
  // xTaskCreate(producer_task, "xyz", 2048 / sizeof(void *), NULL, 2, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
