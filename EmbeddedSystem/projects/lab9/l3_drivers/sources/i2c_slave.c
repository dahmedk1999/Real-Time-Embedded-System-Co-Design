#include "i2c_slave.h"

void i2c2__slave_init(uint8_t assign_slave_add) {
  /* Address register slave mode */
  LPC_I2C0->ADR0 |= (assign_slave_add << 0);
  /*Enable Slave I2EN = 1 AA = 1 (0b1000100) */
  LPC_I2C0->CONSET = 0x44;
}

/**
 * Use memory_index and read the data to *memory pointer
 * return true if everything is well
 */
bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  if (memory_index < 256) {
    *memory = slave_memory[memory_index];
    return true;
  } else {
    printf("Memory out range\n");
    return false;
  }
}

/**
 * Use memory_index to write memory_value
 * return true if this write operation was valid
 */
bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  if (memory_index < 256) {
    slave_memory[memory_index] = memory_value;
    return true;
  } else {
    printf("Memory out range\n");
    return false;
  }
}