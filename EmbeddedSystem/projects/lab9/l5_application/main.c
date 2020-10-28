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
#include "i2c.h"
#include "i2c_slave.h"

typedef struct {
  int16_t x, y, z;
} axis_3d;

/* ----------------------------- ACC Address ----------------------------- */
const uint8_t Slave_address = 0x38;
const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
/* -------------------------- ACC Register Address -------------------------- */
const uint8_t axis_Register = 0x01;
const uint8_t control_Register = 0x2A;
const uint8_t Id_Register = 0x0D;

/* ----------------------------- Configuration ----------------------------- */
const uint8_t Speed_100hz = (1 << 0) | (3 << 3);
const uint32_t p_clock = 96 * 000 * 000;

/* -------------------------------- Function -------------------------------- */
bool acc_checking(void) {
  i2c__write_single(I2C__2, Slave_address, control_Register, Speed_100hz);

  /* Device ID=0x2A Return "True" if matched */
  return (0x2A == i2c__read_single(I2C__2, Slave_address, Id_Register));
}

void i2c0_init(void) {
  /*I/O con Pin config */
  // lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__I2C0);
  // LPC_I2C0->CONCLR = 0x6C; // Clear ALL I2C Flags
  LPC_IOCON->P1_31 |= (1 << 10);
  LPC_IOCON->P1_30 |= (1 << 10);
  gpio__construct_with_function(1, 30, 4);
  gpio__construct_with_function(1, 31, 4);

  i2c__initialize(I2C__0, i2c_speed_hz, clock__get_peripheral_clock_hz());
  // lpc_i2c->CONSET = 0x40;
  puts("done0");
  i2c2__slave_init(0x86);
  puts("done1");
  for (unsigned slave_address = 2; slave_address <= 254; slave_address += 2) {
    if (i2c__detect(I2C__2, slave_address)) {
      printf("I2C slave detected at address: 0x%02X\n", slave_address);
    }
  }
  puts("done3");

  printf("Status: %s\n", i2c__detect(I2C__2, 0x86) ? "Yes" : "NO");
}
/* -------------------------------------------------------------------------- */
axis_3d Get_XYZ_data(void) {
  if (acc_checking) {
    axis_3d sample = {0};
    uint8_t raw_data[6] = {0};
    i2c__read_slave_data(I2C__2, Slave_address, axis_Register, raw_data, sizeof(raw_data));

    /* Combine two uint8_t to one uint16_t ( MSB first )*/
    sample.x = ((uint16_t)raw_data[0] << 8) | raw_data[1];
    sample.y = ((uint16_t)raw_data[2] << 8) | raw_data[3];
    sample.z = ((uint16_t)raw_data[4] << 8) | raw_data[5];

    /* Get Upper 12-bits, remove 4-LSb */
    sample.x = (sample.x >> 4);
    sample.y = (sample.y >> 4);
    sample.z = (sample.z >> 4);
    return sample;
  } else {
    printf("CANNOT Detect ACCELERATION Sensor\n");
  }
}
void Task_XZ(void *P) {
  while (1) {
    axis_3d test1 = {0};
    test1 = Get_XYZ_data();
    printf("X:%d\tZ:%d\n", test1.x, test1.z);
    vTaskDelay(1000);
  }
}

/***************************************** MAIN LOOP *********************************************
**************************************************************************************************/
int main(void) {

  /* ----------------------------- Initialization ----------------------------- */
  puts("Starting RTOS\n");
  sj2_cli__init();
  i2c0_init();

  // printf("Acceleration Status: %s\n", acc_checking() ? "Ready" : "Not Ready");

  /* --------------------------- Written to SD card --------------------------- */
  // sensor_queue = xQueueCreate(1, sizeof(double));
  // WatchDog = xEventGroupCreate();
  // xTaskCreate(Task_XZ, "XZ Position", 2048 / sizeof(void *), NULL, 1, NULL);
  // xTaskCreate(consumer_task, "consumer", 2048 / sizeof(void *), NULL, 2, NULL);
  // xTaskCreate(Watchdog_task, "Watchdog", 2048 / sizeof(void *), NULL, 3, NULL);

  vTaskStartScheduler();
  /* This function never returns unless RTOS scheduler runs out of memory and fails */
  return 0;
}