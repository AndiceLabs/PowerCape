#ifndef __REGISTERS_H__
#define __REGISTERS_H__

enum registers_type {
    REG_FEATURE_MASK,           // 0
    REG_REASON,                 // 1
    REG_RESTART_HOURS,          // 2
    REG_RESTART_MINUTES,        // 3
    REG_RESTART_SECONDS,        // 4
        
    NUM_REGISTERS
};

// STATUS register bits
#define STATUS_POWER_BUTTON     0x01
#define STATUS_POWER_EXTERNAL   0x02
#define STATUS_POWER_TIMER      0x04


void registers_init( void );
uint8_t registers_host_read( uint8_t idx );
void registers_host_write( uint8_t idx, uint8_t data );
uint8_t registers_get( uint8_t index );

#endif  // __REGISTERS_H__

