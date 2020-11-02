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

#include "cli_handlers.h"
#include "decoder_mp3.h"
#include "ff.h"
#include "string.h"
#include "uart_lab.h"

QueueHandle_t Q_songdata;
QueueHandle_t Q_trackname;
TaskHandle_t player_handle;

void mp3_reader_task(void *p) {
  trackname_t name;
  char bytes_512[512];
  UINT br;
  while (1) {
    xQueueReceive(Q_trackname, name, portMAX_DELAY);
    // printf("Received song to play: %s\n", name);

    // open file
    const char *filename = name; // check if accurate
    FIL file;
    FRESULT result = f_open(&file, filename, (FA_READ));
    if (FR_OK == result) {
      while (br != 0) {
        // read 512 bytes from file to bytes_512
        f_read(&file, bytes_512, sizeof(bytes_512), &br);
        xQueueSend(Q_songdata, bytes_512, portMAX_DELAY);

        if (uxQueueMessagesWaiting(Q_trackname)) {
          // printf("New play song request\n");
          break;
        }
      }
      // close file
      f_close(&file);
    } else {
      // printf("Failed to open mp3 file \n");
    }
  }
}

void mp3_player_task(void *p) {
  int counter = 1;
  while (1) {
    char bytes_512[512];
    xQueueReceive(Q_songdata, bytes_512, portMAX_DELAY);
    for (int i = 0; i < 512; i++) {
      // printf("%x \t", bytes_512[i]);
      while (!gpio1__get_level(2, 0)) {
        vTaskDelay(1);
      }
      decoder_send_mp3Data(bytes_512[i]);
    }
    // printf("Received 512 bytes : %d\n", counter);
    counter++;
  }
}

int main(void) {

  Q_trackname = xQueueCreate(1, sizeof(trackname_t));
  /* Send 512 byte at once */
  Q_songdata = xQueueCreate(1, 512);

  // enable GPIO interrupt
  // init_SPI();
  // mp3_setup();
  decoder_setup();

  sj2_cli__init();

  xTaskCreate(mp3_reader_task, "reader", (2024 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "player", (3096 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM,
              &player_handle); // made a change here

  vTaskStartScheduler();
  return 0;
}