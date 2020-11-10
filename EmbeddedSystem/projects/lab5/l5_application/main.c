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

/***************************************** Declaration *********************************************
****************************************************************************************************/
/* Adesto flash 'Manufacturer and Device ID' section */
typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_1;
  uint8_t device_id_2;
  uint8_t extended_device_id;
} adesto_flash_id_s;

adesto_flash_id_s FLASH_ID;

SemaphoreHandle_t spi_bus_mutex;
/***************************************SSP2 Task (Part 1)***************************************
 *@brief:   Read the adesto Flash memory Manufacture ID and Device ID
 *@Note:    Create a ptr[4] array to copy the data out
 ************************************************************************************************/
void spi_task(void *p) {

  /* CLOCK = 1Mhz PreScale = 96*/
  const uint32_t spi_clock_mhz = 1;
  ssp2__setup(spi_clock_mhz);

  /* P1_0-CLK | P1_1-MISO | P1_4-MOSI */
  ssp2_PIN_config();
  uint8_t ptr[4] = {0, 0, 0, 0};

  while (1) {

    /* Start READ */
    ssp2_read(0x9F, 4, ptr);
    FLASH_ID.manufacturer_id = ptr[0];
    FLASH_ID.device_id_1 = ptr[1];
    FLASH_ID.device_id_2 = ptr[2];
    FLASH_ID.extended_device_id = ptr[3];

    /*Errors handle*/
    if (FLASH_ID.manufacturer_id != 0x1F) {
      fprintf(stderr, "Manufacturer ID read failure\n");
      vTaskSuspend(NULL); // Kill this task
    } else {
      fprintf(stderr, "Manufacture ID:  %X \n", FLASH_ID.manufacturer_id);
      fprintf(stderr, "Device_ID1:      %X \n", FLASH_ID.device_id_1);
      fprintf(stderr, "Device_ID2:      %X \n", FLASH_ID.device_id_2);
      fprintf(stderr, "Manufacture ID:  %X \n", FLASH_ID.extended_device_id);
      fprintf(stderr, "\n");
    }
    vTaskDelay(500);
  }
}

/***********************************SSP2 Mutex Task (Part 2b)************************************
 *@brief:   Using one function to run two different task
 *@para:    *Task1 = 1
            *Task2 = 2
 *@Note:    Task parameter will be past into xTaskCreate(....,...)
            --> the demonstrate the purpose of Mutex vs Semaphore
 ************************************************************************************************/
static int *task1 = 1;
static int *task2 = 2;
void Mutex_ID_task(void *p) {

  const int *task = (int *)(p);

  while (1) {
    if (xSemaphoreTake(spi_bus_mutex, 1000)) {

      fprintf(stderr, "Task: %d \n", *task);
      /* CLOCK = 1Mhz PreScale = 96*/
      const uint32_t spi_clock_mhz = 1;
      ssp2__setup(spi_clock_mhz);

      /* P1_0-CLK | P1_1-MISO | P1_4-MOSI */
      ssp2_PIN_config();
      uint8_t ptr[4] = {0, 0, 0, 0};

      /* Start READ */
      ssp2_read(0x9F, 4, ptr);
      FLASH_ID.manufacturer_id = ptr[0];
      FLASH_ID.device_id_1 = ptr[1];
      FLASH_ID.device_id_2 = ptr[2];
      FLASH_ID.extended_device_id = ptr[3];

      /*Errors handle*/
      if (FLASH_ID.manufacturer_id != 0x1F) {
        fprintf(stderr, "Manufacturer ID read failure\n");
        vTaskSuspend(NULL); // Kill this task
      } else {
        fprintf(stderr, "Manufacture ID:  %X \n", FLASH_ID.manufacturer_id);
        fprintf(stderr, "Device_ID1:      %X \n", FLASH_ID.device_id_1);
        fprintf(stderr, "Device_ID2:      %X \n", FLASH_ID.device_id_2);
        fprintf(stderr, "Manufacture ID:  %X \n", FLASH_ID.extended_device_id);
        fprintf(stderr, "\n");
      }
      xSemaphoreGive(spi_bus_mutex);
      vTaskDelay(500);
    }
  }
}

/*******************************SSP2 Task Extra Credit (Part 2a)**********************************
 *@brief:   Perform READ/WRITE any page (128 PAGE -- 256 Byte on Each Page)
 *@Note:    The source code below will print out data from page
            Page 0x000 100---> Test data will run from 0-250
            Page 0x000 200---> Test data will run from 250-0
            erase_data ((uint8_t Page_num) --> every thing on specific page
 ************************************************************************************************/
void task_extra_credit(void *p) {
  const uint32_t spi_clock_mhz = 1;
  ssp2__setup(spi_clock_mhz);

  /* P1_0-CLK | P1_1-MISO | P1_4-MOSI */
  ssp2_PIN_config();

  uint8_t temp[250];
  uint8_t temp2[250];
  // ssp2__write_page(0x000200);

  while (1) {
    // erase_data(0x00);
    // delay__ms(100);

    fprintf(stderr, "Page 0x000 1000 \n");
    ssp2__read_page(0x000100, temp);
    for (int i = 0; i <= 250; i++) {
      fprintf(stderr, "Read Data[%d]: %X \n", i, temp[i]);
    }
    // vTaskDelay(1000);
    fprintf(stderr, "\n");
    fprintf(stderr, "Page 0x000 2000 \n");
    ssp2__read_page(0x000200, temp2);
    for (int i = 0; i <= 250; i++) {
      fprintf(stderr, "Read Data[%d]: %X \n", i, temp2[i]);
    }
    vTaskDelay(1000);
    vTaskSuspend(NULL);
  }
}

/***************************************** MAIN LOOP *********************************************
**************************************************************************************************/
int main(void) {
  spi_bus_mutex = xSemaphoreCreateMutex();
  puts("Starting RTOS");

  /******************************* Part 1 **************************** */
  // xTaskCreate(spi_task, "SPI Task", 4096, NULL, 1, NULL);

  /******************************* Part 2B **************************** */
  // xTaskCreate(Mutex_ID_task, "Task: 1", 1024, &task1, 1, NULL);
  // xTaskCreate(Mutex_ID_task, "Task: 2", 1024, &task2, 1, NULL);

  /****************************** Part 2 + 3 ***************************/
  xTaskCreate(task_extra_credit, "Ex-Credit", 4096, NULL, 1, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
