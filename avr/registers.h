#ifndef __REGISTERS_H__
#define __REGISTERS_H__

enum registers_type {
    REG_MCUSR,                  // 0
    REG_STATUS,                 // 1
    REG_CONTROL,                // 2
    REG_START_ENABLE,           // 3
    REG_START_REASON,           // 4
    REG_RESTART_HOURS,          // 5
    REG_RESTART_MINUTES,        // 6
    REG_RESTART_SECONDS,        // 7
    REG_SECONDS_0,              // 8
    REG_SECONDS_1,              // 9
    REG_SECONDS_2,              // 10
    REG_SECONDS_3,              // 11
        
    NUM_REGISTERS
};


// STATUS register bits
#define STATUS_POWER_GOOD       0x01

// CONTROL register bits
#define CONTROL_CE              0x01
#define CONTROL_LED0            0x02
#define CONTROL_LED1            0x04

// START enable and reason register bits
#define START_BUTTON            0x01
#define START_EXTERNAL          0x02
#define START_PWRGOOD           0x04
#define START_TIMEOUT           0x08


void registers_init( void );
void registers_set_mask( uint8_t index, uint8_t mask );
void registers_clear_mask( uint8_t index, uint8_t mask );
uint8_t registers_get( uint8_t index );
void registers_set( uint8_t idx, uint8_t data );
uint8_t registers_host_read( uint8_t idx );
void registers_host_write( uint8_t idx, uint8_t data );


#endif  // __REGISTERS_H__

