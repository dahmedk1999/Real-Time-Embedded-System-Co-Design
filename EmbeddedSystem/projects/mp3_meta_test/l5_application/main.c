
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
#include "song_handler.h"
#include "string.h"
#include "uart_lab.h"

/* ------------------------------- Global Var ------------------------------- */
typedef struct {
  char Tag[3];
  char Title[30];
  char Artist[30];
  char Album[30];
  char Year[4];
  uint8_t genre;
} mp3_meta;

volatile bool pause = false;
volatile bool menu_open = true;
static uint8_t volume = 0;
volatile uint8_t current_song = 0;
volatile uint8_t control_signal;

/* *************************************************************************** */
/* ------------------------------ Queue handle ------------------------------ */
/* ************************************************************************* */

/* CLI or SONG_Control --> reader_Task */
QueueHandle_t Q_trackname;
/* reader_Task --> player_Task */
QueueHandle_t Q_songdata;

/* *************************************************************************** */
/* ------------------------------- Task Handle ------------------------------ */
/* ************************************************************************* */

TaskHandle_t player_handle;

/* *************************************************************************** */
/* ------------------------ Semaphore Trigger Signal ------------------------ */
/* ************************************************************************* */

SemaphoreHandle_t control_song;
SemaphoreHandle_t play_next;
SemaphoreHandle_t play_previous;
SemaphoreHandle_t next;
SemaphoreHandle_t pause_resume;
SemaphoreHandle_t volume_up;
SemaphoreHandle_t menu;

/* **************************************************************************** */
/* ----------------------------- Control Function ---------------------------- */
/* ************************************************************************** */

/* ----- INTERUPT SERVICE ROUTINE ----- */

static void PIN_and_INTERUPT_setup(); //

/* ----- ISR Button ----- */

void pause_resume_ISR();
void play_next_ISR();     // | Next     |OR| Move down menu |
void play_previous_ISR(); // | Previous |OR| Enter/Exit mnu |
void volume_up_ISR();

/* ----- Helper Funtions ----- */

// READER_TASK
void print_songINFO(char *meta);
void read_songINFO(const char *input_filename);
void print_songProgess(int input_progress);

// BUTTON
void reduce_debounce_ISR(uint8_t port_num, uint8_t pin_num);
void play_next_song(uint8_t next_song);
void play_previous_song(uint8_t previous_song);
void pause_resume_song();
void control_volume();

// PLAYLIST
void update_playlist(uint8_t update_value);
void select_playlist();

/********************************* MP3 READER Task **********************************
 *@brief: Receive song_name(Queue track_name) <--CLI input <handler_general.c>
          Open file = song_name
          Read file --> Store in buffer[size512]
          Send it to player_task(Queue songdata)
          Close file
 *@note:  There are two Queues involved
          ---> QueueHandle_t Q_trackname;
          ---> QueueHandle_t Q_songdata;
          All use PortMax_delay to sleep and wait to wakeup
*************************************************************************************/
static void mp3_reader_task(void *p) {
  trackname_t song_name;
  char TX_buffer512[512];
  UINT br; // byte read
  while (1) {
    xQueueReceive(Q_trackname, song_name, portMAX_DELAY);
    /* ----------------- Extract SongInfo ----------------- */
    const char *Input_fileName = song_name;
    read_songINFO(Input_fileName); // Print_oled included
    /* ---------------- OPEN + READ file ------------------ */
    int song_progress = 1;
    const char *file_name = song_name;
    FIL object_file;
    FRESULT result = f_open(&object_file, file_name, (FA_READ));
    if (FR_OK == result) {
      /* Update NEW "br" for Loopback */
      f_read(&object_file, TX_buffer512, sizeof(TX_buffer512), &br);
      while (br != 0) {
        /* Read-->Store Data (object_file --> buffer) */
        f_read(&object_file, TX_buffer512, sizeof(TX_buffer512), &br);
        xQueueSend(Q_songdata, TX_buffer512, portMAX_DELAY);
        /* New Song request (CLI) */
        if (uxQueueMessagesWaiting(Q_trackname)) {
          break;
        }
        /* Display Clock on OLED */
        print_songProgess(song_progress++);
      }
      /* ------------------- Automate Next ------------------- */
      if (br == 0) {
        song_progress = 1;
        xSemaphoreGive(control_song);
        xSemaphoreGive(play_next);
        control_signal = 0;
        printf("BR == 0\n");
      }
      f_close(&object_file);
    }
    /* ------------------- OPEN + READ Error ------------------- */
    else {
      printf("Failed to open mp3 object_file \n");
    }
  }
}

/******************************* MP3 Player Task *********************************
 *@brief: Receive reader_task(Queue song_data)
          Send every singple byte from buffer to Decoder
 *@note:  Need to wait for Data Request pin (DREQ_HighActive)
 *********************************************************************************/
static void mp3_player_task(void *p) {
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
  }
}

/****************************** MP3 Song control Task ******************************
 *@brief: Control song play list ( Next + Previous + pause/resume  + volume )
 *@note:  Loopback at the end or begin  (Done)
 ***********************************************************************************/
static void mp3_SongControl_task(void *p) {
  while (1) {
    /* Check any Switch is Pressed */
    if (xSemaphoreTake(control_song, portMAX_DELAY)) {
      switch (control_signal) {

      /* -----------------------process NEXT */
      case 0:
        play_next_song(current_song);

      /* -----------------------process PREVIOUS */
      case 1:
        play_previous_song(current_song);

      /* ------------------------------ process PAUSE */
      case 2:
        pause_resume_song();

      /* ----------------------------- process VOLUME */
      case 3:
        control_volume();
      }
    }
  }
}

/****************************** MP3 playlist control Task *******************************
 *@brief: Enter menu + selection( First pressed )
          Exit  menu + execute-->selection ( Second pressed )
 *@note:  Pause/ when enter || AND || Resume + execute new selected song / when exit
 ****************************************************************************************/
static void mp3_PlaylistControl_task() {
  while (1) {
    /* Check Menu Button Press */
    if (xSemaphoreTake(menu, portMAX_DELAY)) {

      vTaskSuspend(player_handle);

      /* ---------------- First Press ---------------- */
      if (menu_open) {
        select_playlist();
      }
      /* ---------------- Second Press ---------------- */
      else {
        /* Deactivate before rewrite or VRAM unstable */
        horizontal_scrolling(0, 7, false);
        vTaskResume(player_handle);
        xQueueSend(Q_trackname, song_list__get_name_for_item(current_song), portMAX_DELAY);
      }
    }
  }
  vTaskDelay(200);
}

/* *************************************************************************** */
/* ------------------------------ MAIN FUNCTION ----------------------------- */
/* ************************************************************************* */
int main(void) {
  /* ----------------------------- Object control */
  // Queue Payload
  Q_trackname = xQueueCreate(1, sizeof(trackname_t));
  Q_songdata = xQueueCreate(1, 512);
  // Song effect
  control_song = xSemaphoreCreateBinary();
  play_next = xSemaphoreCreateBinary();
  play_previous = xSemaphoreCreateBinary();
  pause_resume = xSemaphoreCreateBinary();
  volume_up = xSemaphoreCreateBinary();
  // Playlist menu
  menu = xSemaphoreCreateBinary();
  next = xSemaphoreCreateBinary();

  /* --------------------------- Initialize function */
  PIN_and_INTERUPT_setup();
  decoder_setup();
  sj2_cli__init();


  /* ---------------------------- Testing Song List  */
  song_list__populate();
  for (size_t song_number = 0; song_number < song_list__get_item_count(); song_number++) {
    printf("-->%2d: %s\n", (1 + song_number), song_list__get_name_for_item(song_number));
  }
  /* without mp3 */
  open_directory_READ();

  /* ------------------------------- xTaskCreate */
  xTaskCreate(mp3_reader_task, "task_reader", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "task_player", (2048) / sizeof(void *), NULL, PRIORITY_MEDIUM, &player_handle);
  xTaskCreate(mp3_SongControl_task, "Song_control", (2048 * 2) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_PlaylistControl_task, "menu", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  /* Never retrun unless RTOS scheduler runs out of memory and fails */
  vTaskStartScheduler();
  return 0;
}

/* -------------------------------------------------------------------------- */
/* ----------------------------- Interrupt Setup ---------------------------- */
/* -------------------------------------------------------------------------- */

void PIN_and_INTERUPT_setup() {
  /*PIN setup*/
  LPC_IOCON->P0_25 &= ~(0b111); // NEXT
  gpio1__set_as_input(0, 25);
  LPC_IOCON->P0_26 &= ~(0b111); // PREVIOUS
  gpio1__set_as_input(0, 26);
  LPC_IOCON->P0_30 &= ~(0b111); // Volume control
  gpio1__set_as_input(0, 30);
  LPC_IOCON->P0_29 &= ~(0b111); // Pause
  gpio1__set_as_input(0, 29);
  LPC_IOCON->P0_22 &= ~(0b111); // multiplex
  gpio1__set_as_input(0, 22);
  /* Please check the gpio_isr.h */
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, "INTR Port 0");
  gpio0__attach_interrupt(30, GPIO_INTR__FALLING_EDGE, volume_up_ISR);     // Volume
  gpio0__attach_interrupt(25, GPIO_INTR__FALLING_EDGE, play_next_ISR);     // Next
  gpio0__attach_interrupt(26, GPIO_INTR__FALLING_EDGE, play_previous_ISR); // Previous
  gpio0__attach_interrupt(29, GPIO_INTR__FALLING_EDGE, pause_resume_ISR);  // Pause/resume
}

void play_next_ISR() {
  reduce_debounce_ISR(0, 25);
  /*  menu move up */
  if (gpio1__get_level(0, 22)) {
    xSemaphoreGiveFromISR(next, NULL);
  }
  /*  play next */
  if (!gpio1__get_level(0, 22)) {
    xSemaphoreGiveFromISR(control_song, NULL);
    xSemaphoreGiveFromISR(play_next, NULL);
    control_signal = 0;
  }
}

void play_previous_ISR() {
  reduce_debounce_ISR(0, 26);
  /* toggle menu (Pause + Enter | Resume + Execute) */
  menu_open = !menu_open;
  if (gpio1__get_level(0, 22)) {
    xSemaphoreGiveFromISR(menu, NULL);
  }
  /*  play previous */
  if (!gpio1__get_level(0, 22)) {
    xSemaphoreGiveFromISR(control_song, NULL);
    xSemaphoreGiveFromISR(play_previous, NULL);
    control_signal = 1;
  }
}

void pause_resume_ISR() {
  reduce_debounce_ISR(0, 29);
  xSemaphoreGiveFromISR(control_song, NULL);
  xSemaphoreGiveFromISR(pause_resume, NULL);
  control_signal = 2;
}

void volume_up_ISR() {
  reduce_debounce_ISR(0, 30);
  xSemaphoreGiveFromISR(control_song, NULL);
  xSemaphoreGiveFromISR(volume_up, NULL);
  control_signal = 3;
}

/* -------------------------------------------------------------------------- */
/* ----------------------------- HELPER FUNCTION ---------------------------- */
/* -------------------------------------------------------------------------- */

/* ---------------- Display + Update (playlist menu) ---------------- */
void update_playlist(uint8_t update_value) {
  /* songName without .mp3 version */
  oled_clear_page(page_0, page_7);
  for (int page = 0; page < 5; page++) {
    oled_print(get_songName_on_INDEX(page + update_value), page, 0, 0);
  }
}

/* ---------------- Handle Screen Playlist transition ---------------- */
void select_playlist() {
  uint8_t list_index = 0;
  uint8_t song_select = 0;
  const int LCD_row_display = 5;
  int list_max_size = (song_list__get_item_count());
  update_playlist(0);
  while (menu_open) {
    if (xSemaphoreTake(next, 0)) {
      song_select++;
      list_index++;
      /* Update playlist (use Modulus Operator) */
      if (list_index % LCD_row_display == 0 && list_index != 0) {
        update_playlist(list_index);
        vTaskDelay(10);
      }
    }
    horizontal_scrolling(song_select, song_select, true);
    /* Conditioning for resize */
    if (song_select == LCD_row_display) {
      song_select = 0;
    }
    /* Loopback on last item */
    if (list_index == list_max_size) {
      list_index = 0;
      song_select = 0;
      update_playlist(list_index);
    }
    current_song = list_index;
  }
}

/* ---------------- Reduce debounce rate ---------------- */
void reduce_debounce_ISR(uint8_t port_num, uint8_t pin_num) {
  uint16_t delay = 0;
  while (gpio1__get_level(port_num, pin_num)) {
    delay = delay + 100;
  }
  while (!gpio1__get_level(port_num, pin_num) && (delay != 0)) {
    delay--;
  }
}

/* ---------------- Play next song ---------------- */
void play_next_song(uint8_t next_song) {
  if (xSemaphoreTake(play_next, 10)) {
    /* Loopback when hit last song */
    if (next_song == song_list__get_item_count()) {
      next_song = 0;
      xQueueSend(Q_trackname, song_list__get_name_for_item(next_song++), portMAX_DELAY);
      current_song = next_song;
    } /*Not Fist song */
    else if (next_song != 0) {
      xQueueSend(Q_trackname, song_list__get_name_for_item(++next_song), portMAX_DELAY);
      current_song = next_song;
    } /* First song */
    else {
      // next_song = 0;
      xQueueSend(Q_trackname, song_list__get_name_for_item(next_song++), portMAX_DELAY);
      current_song = next_song;
    }
  }
}
/* ---------------- Play previous song ----------------  */
void play_previous_song(uint8_t previous_song) {
  if (xSemaphoreTake(play_previous, 10)) {
    /* Loopback when hit first song */
    if (previous_song == 0) {
      previous_song = song_list__get_item_count();
      xQueueSend(Q_trackname, song_list__get_name_for_item(--previous_song), portMAX_DELAY);
      current_song = previous_song;
    } /* Keep decrement */
    else {
      xQueueSend(Q_trackname, song_list__get_name_for_item(--previous_song), portMAX_DELAY);
      current_song = previous_song;
    }
  }
}
/* ---------------- PAUSE || RESUME ----------------  */
void pause_resume_song() {
  if (xSemaphoreTake(pause_resume, 10)) {
    vTaskDelay(200);
    pause = !pause;
    if (pause) {
      vTaskSuspend(player_handle);
    } else {
      vTaskResume(player_handle);
    }
  }
}

/* ---------------- Volume Up ( loop back ) ---------------- */
void control_volume() {
  if (xSemaphoreTake(volume_up, 10)) {
    if (volume == 13) {
      volume = 0;
    }
    vTaskDelay(150);
    static char Vol_level[10];
    sprintf(Vol_level, "Vol:%2d", (13 - volume));
    oled_print(Vol_level, page_7, 4, 0);
    memset(Vol_level, 0, 10);
    set_volume(volume++);
    vTaskDelay(150);
  }
}

/* -------------------------------------------------------------------------- */
/* --------- Process last_128 Byte file.mp3 + Print playing progress -------- */
/* -------------------------------------------------------------------------- */

/* ---------- Print song INFO on oled screen ---------- */
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
  oled_print("Init dummy", page_0, 0, init);
  oled_clear_page(0, 6); // oled clear require some dummy
  oled_print(song_INFO.Title, page_0, 0, init);
  oled_print(song_INFO.Artist, page_3, 0, 0);
  oled_print(song_INFO.Album, page_5, 0, 0);
}

/* ---------- Read File to extract song INFO ---------- */
void read_songINFO(const char *input_filename) {
  /* ----- OPEN file ----- */
  FIL object_file;
  FRESULT result = f_open(&object_file, input_filename, (FA_READ));
  UINT br; // byte read
  char meta_128[128];
  /* --------------------- READ Song INFO ---------------------- */
  /***************************************************************
   * f_lseek(ptr_objectFile, ptr_READ/WRITE[top-->bottom])       *
   * | --> sizeof(mp3_file) - last_128[byte]                     *
   * | ----> Set READ pointer                                    *
   * | ------> Extract meta file                                 *
   * | --------> Put the READ pointer back to [0]                *
   * | ----------> Close file                                    *
   ***************************************************************/
  if (FR_OK == result) {
    f_lseek(&object_file, f_size(&object_file) - (sizeof(char) * 128));
    f_read(&object_file, meta_128, sizeof(meta_128), &br);
    print_songINFO(meta_128);
    f_lseek(&object_file, 0);
    f_close(&object_file);
  } else {
    printf("Extact mp3 file fail\n");
  }
}

/* ---------- Calculate the Song Progress ---------- */
void print_songProgess(int input_progress) {
  static uint8_t speed = 35; // 78.964;
  uint8_t second = input_progress / speed;
  uint8_t minute = input_progress / (speed * 60);
  /* -------------- Calculate Real-Time Playing -------------- */
  /**************************************************************
   * song_progress: Total number of time tranfer 512B_byte      *
   * Time         : Song Duration                               *
   * Speed        : How many time 512_byte is transfer/second   *
   *------------------------For Example-------------------------*
   *  Time: 3:45 min                                            *
   *  song_progress: 17767 times transfer 512_byte              *
   *  The estimation will be ---> 17767/225 = 78.964            *
   *  Then 78.964/2 = 39(+-5) (cause we have 2 task)            *
   *  This measurement is not 100% matching but 95%             *
   *  However, It just apply for 128 bit/rate song              *
   * ---->with 320 bit/rate song it is run faster               *
   * ---->Same duration but diffrent size of mp3 file           *
   * ---->Speed change  (need to work on this)                  *
   **************************************************************/
  if (second > 60) {
    second = second - (minute * 60);
  }
  static char playing_time[30];
  sprintf(playing_time, "[%02d:%02d]", minute, second);
  oled_print(playing_time, page_7, 0, 0);
  memset(playing_time, 0, 30);
}
