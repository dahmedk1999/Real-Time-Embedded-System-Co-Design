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

#include "event_groups.h"
#include "ff.h"

/* -------------------------------------------------------------------------- */
/* ---------------------- WRITE + READ (SD_CARD) Sample --------------------- */
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
 *@brief: Getting data from Acceleration sensor
          Find Avg of 100 Sample
          Send it to Queue
 *@Note:  Consumer and Producer have the same priority
          -->xQueueSend (..,.., 0 ) + Sleep 1000mS
          -->xQueueReceive(.., .., PortMax_Delay) + No Sleep
          -->WatchDog verify xEventGroupWaitBits(..,..,..,...,2000 ms)

          Consumer ---> Save Queue data to file.txt
          WatchDog ---> Save LogInfo data to watch_dog.txt
 ******************************************************************************************/
typedef struct {
  acceleration__axis_data_s sample[100];
  double sum_y;
  double avg_y;
} data;

static QueueHandle_t sensor_queue;
static EventGroupHandle_t WatchDog;
static TickType_t counting;
static TickType_t dog_counter1, dog_counter2;
/* xEventGroupSetBits */
uint8_t producer = 0x01, consumer = 0x02;
/* xEventGroupWaitBits */
uint8_t verified = 0x03;

/* -------------------------------------------------------------------------- */
/* ---------------------------------- SEND ---------------------------------- */
static void producer_task(void *P) {
  /*Initialize sensor*/
  printf("Sensor Status: %s\n", acceleration__init() ? "Ready" : "Not Ready");
  data data1;
  while (1) {

    for (int i = 0; i < 100; i++) {
      data1.sample[i] = acceleration__get_data();
      data1.sum_y += data1.sample[i].y;
    }
    data1.avg_y = data1.sum_y / 100;
    printf("EnQueue: %s\n", xQueueSend(sensor_queue, &data1.avg_y, 0) ? "T" : "F");
    xEventGroupSetBits(WatchDog, producer);
    data1.sum_y = 0.000;

    vTaskDelay(1000);
  }
}

/* -------------------------------------------------------------------------- */
/* --------------------------------- RECEIVE -------------------------------- */
static void consumer_task(void *P) {
  /*  Create ptr *file_name + file object */
  const char *filename = "Sensor.txt";
  FIL file;
  UINT bytes_written = 0;
  /* File Open IF exist file or Create IF NOT exist */
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_CREATE_ALWAYS));
  double receive;
  while (1) {
    if (xQueueReceive(sensor_queue, &receive, portMAX_DELAY)) {
      if (FR_OK == result) {
        static char string[64];
        /* Keep track Time execution */
        counting = xTaskGetTickCount();
        sprintf(string, "%ld mS <-> Avg: %.3f\n", counting, receive);
        if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
          /* Flush Cache + Save progress*/
          f_sync(&file);
        } else {
          printf("ERROR: Failed to write data to file\n");
        }
      } else {
        printf("ERROR: Failed to open: %s\n", filename);
      }
    }
    printf("%ld mS <-> Avg: %.3f\n", counting, receive);
    xEventGroupSetBits(WatchDog, consumer);
  }
}

/* -------------------------------------------------------------------------- */
/* --------------------------------- VERIFY --------------------------------- */
void Watchdog_task(void *P) {

  /*  Create ptr *file_name + file object */
  const char *filename1 = "watch_dog.txt";
  FIL file;
  UINT bytes_written = 0;
  /* File Open IF exist file or Create IF NOT exist */
  FRESULT result = f_open(&file, filename1, (FA_WRITE | FA_CREATE_ALWAYS));

  while (1) {
    /* Set Clear + AND Gate logic for bit pattern - Wait for 1000ms */
    uint8_t expected_value = xEventGroupWaitBits(WatchDog, verified, pdTRUE, pdTRUE, 2000); // 3
    /* Expect_Value[0] = 0x3 --> Expect_Value[1++] = 0x00 */
    printf("WatchDog verify: %s\tReturn_Value: %d \n ", (expected_value == verified) ? "T" : "F", expected_value);

    /* ---------------------------- BOTH TASK HEALTHY --------------------------- */
    if ((expected_value == verified)) {
      printf("Both Task Healthy\n\n");

      /* Save to .txt file  */
      if (FR_OK == result) {
        static char string[64];
        dog_counter1 = xTaskGetTickCount();
        sprintf(string, "Verified at time: %ldmS\n", dog_counter1);
        if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
          /* Flush Cache + Save progress*/
          f_sync(&file);
        } else {
          printf("ERROR: Failed to write data to file\n");
        }
      } else {
        printf("ERROR: Failed to open: %s\n", filename1);
      }
    }
    /* ----------------------------- CONSUMER ERROR ----------------------------- */
    else if (expected_value == 1) {
      printf("C_Task Suspend\n\n");

      /* Save to .txt file  */
      if (FR_OK == result) {
        static char string[64];
        dog_counter2 = xTaskGetTickCount();
        sprintf(string, "Consumer Error at time: %ldmS\n ", dog_counter2);
        if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
          /* Flush Cache + Save progress*/
          f_sync(&file);
        } else {
          printf("ERROR: Failed to write data to file\n");
        }
      } else {
        printf("ERROR: Failed to open: %s\n", filename1);
      }
    }
    /* ----------------------------- PRODUCER ERROR ----------------------------- */
    else {
      printf("P_Task Suspend\n\n");

      /* Save to .txt file  */
      if (FR_OK == result) {
        static char string[64];
        dog_counter2 = xTaskGetTickCount();
        sprintf(string, "Producer Error at time: %ldmS\n ", dog_counter2);
        if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
          /* Flush Cache + Save progress*/
          f_sync(&file);
        } else {
          printf("ERROR: Failed to write data to file\n");
        }
      } else {
        printf("ERROR: Failed to open: %s\n", filename1);
      }
    }
  }
}

/***************************************** MAIN LOOP *********************************************
**************************************************************************************************/
int main(void) {

  /* ----------------------------- Initialization ----------------------------- */
  puts("Starting RTOS\n");
  sj2_cli__init();

  /* --------------------------- Written to SD card --------------------------- */
  // sensor_queue = xQueueCreate(1, sizeof(double));
  // WatchDog = xEventGroupCreate();
  // xTaskCreate(producer_task, "producer", 2048 / sizeof(void *), NULL, 2, NULL);
  // xTaskCreate(consumer_task, "consumer", 2048 / sizeof(void *), NULL, 2, NULL);
  // xTaskCreate(Watchdog_task, "Watchdog", 2048 / sizeof(void *), NULL, 3, NULL);

  vTaskStartScheduler();
  /* This function never returns unless RTOS scheduler runs out of memory and fails */
  return 0;
}