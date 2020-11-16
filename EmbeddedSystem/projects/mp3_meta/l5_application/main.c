
#include "FreeRTOS.h"
#include "adc.h"
#include "board_io.h"
#include "common_macros.h"
// #include "delay.h"
#include "gpio.h"
#include "gpio_isr.h"

#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"

#include "oled.h"
#include "queue.h"
#include "semphr.h"
#include "sj2_cli.h"
// #include "ssp2_lab.h"
#include "task.h"
#include <stdio.h>

#include "cli_handlers.h"
#include "decoder_mp3.h"
#include "ff.h"
#include "song_handler.h"
#include "string.h"
// #include "uart_lab.h"

/* -------------------------------------------------------------------------- */
typedef struct {
  char Tag[3];
  char Title[30];
  char Artist[30];
  char Album[30];
  char Year[4];
  uint8_t genre;
} mp3_meta;

void print_songINFO(char *meta) {
  // int index_counter = 0;
  // char meta_array[128] = {"0"};
  uint8_t title_counter = 0;
  uint8_t artist_counter = 0;
  uint8_t album_counter = 0;
  uint8_t year_counter = 0;
  /* Require Array Init (or Crash Oled driver) */
  mp3_meta song_INFO = {0};
  for (int i = 0; i < 128; i++) {
    if ((((int)(meta[i]) > 47) && ((int)(meta[i]) < 58)) ||  // number
        (((int)(meta[i]) > 64) && ((int)(meta[i]) < 91)) ||  // ALPHABET
        (((int)(meta[i]) > 96) && ((int)(meta[i]) < 123)) || // alphabet
        ((int)(meta[i])) == 32) {                            // Space
      char character = (int)(meta[i]);
      if (i < 3)
        song_INFO.Tag[i] = character;
      else if (i > 2 && i < 33)
        song_INFO.Title[title_counter++] = character;
      else if (i > 32 && i < 63)
        song_INFO.Artist[artist_counter++] = character;
      else if (i > 62 && i < 93)
        song_INFO.Album[album_counter++] = character;
      else if (i > 92 && i < 97)
        song_INFO.Year[year_counter++] = character;
      /*Testing*/
      // meta_array[index_counter] = meta[i];
      // index_counter = index_counter + 1;
      // printf("t[%d]:  %c\n ", i, meta[i]);
    }
  }
  /* ----- OLED screen ----- */
  oled_print(song_INFO.Title, page_0, init);
  oled_print(song_INFO.Artist, page_3, 0);
  oled_print(song_INFO.Album, page_5, 0);
  // oled_print(song_INFO.Year, page_7, 0);
}
/* ------------------------------ Queue handle ------------------------------ */
/* CLI or SONG_Control --> reader_Task */
QueueHandle_t Q_trackname;
/* reader_Task --> player_Task */
QueueHandle_t Q_songdata;

/* ------------------------------- Task Handle ------------------------------ */
TaskHandle_t player_handle;

/* ------------------------ Semaphore Trigger Signal ------------------------ */
SemaphoreHandle_t play_next;
SemaphoreHandle_t play_previous;
SemaphoreHandle_t pause_resume;
SemaphoreHandle_t next_previous;

volatile bool pause = true;
/* ----------------------------- Control Function ---------------------------- */
/*INTERUPT SERVICE ROUTINE */
static void interupt_setup(); //

/* ISR Button */
static void pause_resume_ISR();
static void play_next_ISR();
static void play_previous_ISR();

/* button_Task */
static void pause_resume_Button(); //

/************************************* MP3 READER Task  **********************************
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
static void mp3_reader_task(void *p) {
  trackname_t song_name;
  char TX_buffer512[512];
  UINT br; // byte read
  while (1) {
    xQueueReceive(Q_trackname, song_name, portMAX_DELAY);
    int distance = 1;
    /* ----- OPEN file ----- */
    const char *file_name = song_name;
    FIL object_file;
    FRESULT result = f_open(&object_file, file_name, (FA_READ));

    /* ----------------------------- READ Song INFO ----------------------------- */
    static char meta_128[128];
    /*************************************************************
     * f_lseek( ptr_objectFile, ptr_READ/WRITE[top-->bottom] )
     * | --> sizeof(mp3_file) - last_128[byte]
     * | ----> Set READ pointer
     * | ------> Extract meta file
     * | --------> Put the READ pointer back to [0]
     **************************************************************/
    f_lseek(&object_file, f_size(&object_file) - (sizeof(char) * 128));
    f_read(&object_file, meta_128, sizeof(meta_128), &br);
    print_songINFO(meta_128);
    f_lseek(&object_file, 0);
    /* -------------------------------------------------------------------------- */
    if (FR_OK == result) {
      /* Update NEW "br" for Loopback */
      f_read(&object_file, TX_buffer512, sizeof(TX_buffer512), &br);
      while (br != 0) {
        /* Read-->Store Data (object_file --> buffer) */
        f_read(&object_file, TX_buffer512, sizeof(TX_buffer512), &br);
        xQueueSend(Q_songdata, TX_buffer512, portMAX_DELAY);
        /* New Song request (CLI) */
        if (uxQueueMessagesWaiting(Q_trackname)) {
          // printf("New song request\n");
          break;
        }
        /* ----------------------- Calculate Real-Time Playing ----------------------- */
        /**************************************************************
         * Distance: Total number of time tranfer 512B_byte           *
         * Time    : Song Duration                                    *
         * Speed   : How many time 512_byte is transfer/second        *
         * ------------------------For Example------------------------*
         *  Time: 3:45 min                                            *
         *  Distance: 17767 times transfer 512_byte                   *
         *  The estimation will be ---> 17767/225 = 78.964            *
         *  Then 78.964/2 = 39(+-5) (cause we have 2 task)            *
         *  This measurement is not 100% matching but 95%             *
         *  However, It just apply for 128 bit/rate song              *
         * ---->with 320 bit/rate song it is run faster               *
         * ---->Same duration but diffrent size of mp3 file           *
         * ---->Speed change  (need to work on this)                  *
         **************************************************************/
        static uint8_t speed = 35; // 78.964;
        uint8_t second = distance / speed;
        uint8_t minute = distance / (speed * 60);
        if (second > 60) {
          second = second - (minute * 60);
        }
        static char playing_time[30];
        sprintf(playing_time, "(@.@)%2d:%2d", minute, second);
        oled_print(playing_time, page_7, 0);
        memset(playing_time, 0, 30);
        distance++;
      }
      /* ----- Automate Next ----- */
      if (br == 0) {
        distance = 1;
        xSemaphoreGive(next_previous);
        xSemaphoreGive(play_next);
        // printf("BR == 0\n");
      }
      f_close(&object_file);
    } else {
      printf("Failed to open mp3 object_file \n");
    }
  }
}
/************************************* MP3 Player Task ************************************
 *@brief: Receive reader_task(Queue song_data)
          Send every singple byte from buffer to Decoder
 *@note:  Need to wait for Data Request pin (DREQ_HighActive)
 ******************************************************************************************/
static void mp3_player_task(void *p) {
  int counter = 1;
  while (1) {
    /* Receive songdata ---> Transfer to DECODER  */
    char RX_buffer512[512];
    xQueueReceive(Q_songdata, RX_buffer512, portMAX_DELAY);
    for (int i = 0; i < 512; i++) {
      /* Data Request (Activated) */
      while (!get_DREQ_HighActive()) {
        vTaskDelay(1);
      }
      decoder_send_mp3Data(RX_buffer512[i]);
    }
    // printf("Buffer Transmit: %d (times)\n", counter);
    counter++;
  }
}

/******************************** MP3 Song control Task ***********************************
 *@brief: Control song play list (Loopback + Next + Previous)
 *@note:  Loopback at the end (Done)
          Next song (Done)
          Previous (???)
 ******************************************************************************************/

static void mp3_SongControl_task(void *p) {
  volatile size_t song_index = 0;
  song_list__populate();
  while (1) {
    /* Check any Switch is Pressed */
    if (xSemaphoreTake(next_previous, portMAX_DELAY)) {
      /* -----------------------process NEXT */
      if (xSemaphoreTake(play_next, 10)) {
        while (gpio1__get_level(0, 25)) {
          vTaskDelay(10);
        }
        /* Loopback when hit last song */
        if (song_index == song_list__get_item_count()) {
          song_index = 0;
        }
        xQueueSend(Q_trackname, song_list__get_name_for_item(song_index), portMAX_DELAY);
        song_index++;
      }
      /* -----------------------process PREVIOUS */
      else if (xSemaphoreTake(play_previous, 10)) {
        while (gpio1__get_level(0, 26)) {
          vTaskDelay(10);
        }
        /* Loopback when hit first song */
        if (song_index == 0) {
          song_index = song_list__get_item_count();
        }
        xQueueSend(Q_trackname, song_list__get_name_for_item(song_index), portMAX_DELAY);
        song_index--;
      }
    }
    vTaskDelay(100);
  }
}

/* -------------------------------------------------------------------------- */

int main(void) {

  /* ----------------------------- Object control */

  Q_trackname = xQueueCreate(1, sizeof(trackname_t));
  Q_songdata = xQueueCreate(1, 512);
  next_previous = xSemaphoreCreateBinary();
  play_next = xSemaphoreCreateBinary();
  play_previous = xSemaphoreCreateBinary();
  pause_resume = xSemaphoreCreateBinary();

  /* --------------------------- Initialize function */
  interupt_setup();
  decoder_setup();
  sj2_cli__init();

  /* ---------------------------- Testing Song List  */
  song_list__populate();
  for (size_t song_number = 0; song_number < song_list__get_item_count(); song_number++) {
    printf("-->%2d: %s\n", (1 + song_number), song_list__get_name_for_item(song_number));
  }

  /* ------------------------------- xTaskCreate  */
  xTaskCreate(mp3_reader_task, "task_reader", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "task_player", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, &player_handle);
  xTaskCreate(mp3_SongControl_task, "Song_control", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(pause_resume_Button, "Pause_task", (2048) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  vTaskStartScheduler();
  return 0;
}

/* -------------------------------------------------------------------------- */
/* ----------------------------- Button Function ---------------------------- */
/* -------------------------------------------------------------------------- */

/* ----------------------------- Interrupt Setup ---------------------------- */

static void interupt_setup() {
  /*PIN setup*/
  LPC_IOCON->P0_25 &= ~(0b111); // NEXT
  gpio1__set_as_input(0, 25);
  LPC_IOCON->P0_26 &= ~(0b111); // PREVIOUS
  gpio1__set_as_input(0, 26);
  /* Please check the gpio_isr.h */
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, "INTR Port 0");
  // gpio0__attach_interrupt(30, GPIO_INTR__FALLING_EDGE, play_next_ISR);
  gpio0__attach_interrupt(25, GPIO_INTR__RISING_EDGE, play_next_ISR);
  gpio0__attach_interrupt(26, GPIO_INTR__RISING_EDGE, play_previous_ISR);
  gpio0__attach_interrupt(29, GPIO_INTR__FALLING_EDGE, pause_resume_ISR);
}

static void play_next_ISR() {
  xSemaphoreGiveFromISR(next_previous, NULL);
  xSemaphoreGiveFromISR(play_next, NULL);
}
static void play_previous_ISR() {
  xSemaphoreGiveFromISR(next_previous, NULL);
  xSemaphoreGiveFromISR(play_previous, NULL);
}
static void pause_resume_ISR() { xSemaphoreGiveFromISR(pause_resume, NULL); }

/* --------------------------------- Button --------------------------------- */
static void pause_resume_Button() {
  while (1) {
    if (xSemaphoreTake(pause_resume, portMAX_DELAY)) {
      if (pause) {
        vTaskSuspend(player_handle);
        pause = false;
      } else {
        vTaskResume(player_handle);
        pause = true;
      }
    }
  }
}

/* -------------------------------------------------------------------------- */