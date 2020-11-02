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

/********************************* MP3 READER Task (Part 1) *******************************
 *@brief: Receive song_name(Queue track_name) <--CLI input <handler_general.c>
          Open file = song_name
          Read file --> Store in buffer[size512]
          Send it to player_task(Queue songdata)
          Close file
 *@note:  There are two Queues involved
          ---> QueueHandle_t Q_trackname;
          ---> QueueHandle_t Q_songdata;
          All use PortMax_delay to sleep and wait to wakeup
 ******************************************************************************************/
void mp3_reader_task(void *p) {
  trackname_t song_name;
  char TX_buffer512[512];
  UINT br;
  while (1) {
    xQueueReceive(Q_trackname, song_name, portMAX_DELAY);
    oled_print(song_name); // Test it on the Oled Screen
    const char *file_name = song_name;
    FIL object_file;
    FRESULT result = f_open(&object_file, file_name, (FA_READ));
    if (FR_OK == result) {
      while (br != 0) {
        /* Read-->Store Data (object_file --> buffer) */
        f_read(&object_file, TX_buffer512, sizeof(TX_buffer512), &br);
        xQueueSend(Q_songdata, TX_buffer512, portMAX_DELAY);
        /* New Song request ???*/
      }
      f_close(&object_file);
    } else {
      printf("Failed to open mp3 object_file \n");
    }
  }
}
/********************************* MP3 Player Task (Part 1) *******************************
 *@brief: Receive reader_task(Queue song_data)
          Send every singple byte from buffer to Decoder
 *@note:  Need to wait for Data Request pin (DREQ_HighActive)
 ******************************************************************************************/
void mp3_player_task(void *p) {
  int counter = 1;
  while (1) {
    char RX_buffer512[512];
    xQueueReceive(Q_songdata, RX_buffer512, portMAX_DELAY);
    for (int i = 0; i < 512; i++) {
      /* Data Request (Activated) */
      while (!get_DREQ_HighActive()) {
        vTaskDelay(1);
      }
      decoder_send_mp3Data(RX_buffer512[i]);
      // printf("%x \t", RX_buffer512[i]);
    }
    // printf("Buffer Transmit: %d (times)\n", counter);
    counter++;
  }
}

int main(void) {

  Q_trackname = xQueueCreate(1, sizeof(trackname_t));
  /* Send 512-bytes at once */
  Q_songdata = xQueueCreate(1, 512);

  decoder_setup();
  sj2_cli__init();

  xTaskCreate(mp3_reader_task, "task_reader", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "task_player", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, &player_handle);

  vTaskStartScheduler();
  return 0;
}