#pragma once

#include "lpc40xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "gpio.h"
#include "i2c.h"

/* ------------------------- Initial I2C__1 as Slave ------------------------ */

volatile uint8_t slave_memory[256];

void i2c1__slave_init(uint8_t assign_slave_add);

/* ----------------------------- Slave Transmit ----------------------------- */

/**
 * Use memory_index and read the data to *memory pointer
 * return true if everything is well
 */
bool i2c_slave_transmit__read_memory(uint8_t memory_index, uint8_t *memory);

/* ------------------------------ Slave Receive ----------------------------- */

/**
 * Use memory_index to write memory_value
 * return true if this write operation was valid
 */
bool i2c_slave_receive__write_memory(uint8_t memory_index, uint8_t memory_value);
