#include "i2c_slave.h"

/*================================I2C_1 Slave Inital ================================
*@brief:    Config I2C__1 as Slave (P0_1=SCLK P0_0=SDA )
*@para:     assign_slave_add [in]
*@return:   No Return
*@Note:     Check "i2c.h" for basic config
            --> Require Open-drain Pin mode (IOCON) + I2C function(IOCON)
            --> address register take effect with SLAVE MODE (I2CONSET = 0x44)
=====================================================================================*/
void i2c1__slave_init(uint8_t assign_slave_add) {
  const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;

  /*I/O con Pin config */
  LPC_IOCON->P0_1 |= (1 << 10);
  LPC_IOCON->P0_0 |= (1 << 10);
  gpio__construct_with_function(0, 0, 3); // SDA
  gpio__construct_with_function(0, 1, 3); // SCLK

  /* Check the "i2c.h" */
  i2c__initialize(I2C__1, i2c_speed_hz, clock__get_peripheral_clock_hz());

  /* Address register slave mode */
  LPC_I2C1->ADR0 |= (assign_slave_add << 0);
  /*Enable Slave I2EN = 1 AA = 1 (0b1000100) */
  LPC_I2C1->CONSET = 0x44;
  printf("I2C_1 Slave(0x%X): %s\n", assign_slave_add, i2c__detect(I2C__2, assign_slave_add) ? "Ready" : "NOT Ready");
}

/*================================I2C_1 Slave Receive ===============================
*@brief:    Slave Receive <--- Master Transmit
*@para:     register_index [in]
            memory_ptr [in]
*@return:   Status: (TRUE | FALSE)
*@Note:     volatile uint8_t slave_memory[256];
            _____________________________________
            State Involved: |0x60 | 0x80 | 0xA0 |
=====================================================================================*/
bool i2c_slave_receive__write_memory(uint8_t register_index, uint8_t memory_value) {
  if (register_index < 256) {
    slave_memory[register_index] = memory_value;
    return true;
  } else {
    printf("Memory out range\n");
    return false;
  }
}

/*================================I2C_1 Slave Transmit ===============================
*@brief:    Slave Tramsmit ---> Master Receive
*@para:     register_index [in]
            memory_ptr [out]
*@return:   Status: (TRUE | FALSE)
*@Note:     volatile uint8_t slave_memory[256];
            ____________________________________________
            State Involved: |0xA8 | 0xB8 | 0xC0 | 0xC8 |
=====================================================================================*/
bool i2c_slave_transmit__read_memory(uint8_t register_index, uint8_t *memory_ptr) {
  if (register_index < 256) {
    *memory_ptr = slave_memory[register_index];
    return true;
  } else {
    printf("Memory out range\n");
    return false;
  }
}
