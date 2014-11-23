#ifndef __BB_I2C_H__
#define __BB_I2C_H__

#define     MCP_ADDR    0x5E

void bb_i2c_init( void );
uint8_t bb_i2c_read( uint8_t addr, uint8_t *buf, uint8_t len );
uint8_t bb_i2c_write( uint8_t addr, uint8_t *buf, uint8_t len );

#endif  // __BB_I2C_H__
