#pragma once

#include "lpc40xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* ------------------------------ Slave Initial ----------------------------- */

volatile uint8_t slave_memory[256];

void i2c2__slave_init(uint8_t assign_slave_add);

/* ----------------------------- Slave Transmit ----------------------------- */

/**
 * Use memory_index and read the data to *memory pointer
 * return true if everything is well
 */
bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory);

/* ------------------------------ Slave Receive ----------------------------- */

/**
 * Use memory_index to write memory_value
 * return true if this write operation was valid
 */
bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value);