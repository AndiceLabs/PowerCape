#ifndef __REGISTERS_H__
#define __REGISTERS_H__

enum registers_type {
    REG_FEATURE_MASK,           // 0
    REG_START_ENABLE,           // 1
    REG_START_REASON,           // 2
    REG_RESTART_HOURS,          // 3
    REG_RESTART_MINUTES,        // 4
    REG_RESTART_SECONDS,        // 5
        
    NUM_REGISTERS
};

// START enable and reason register bits
#define START_BUTTON            0x01
#define START_EXTERNAL          0x02
#define START_PWRGOOD           0x04
#define START_TIMEOUT           0x08


void registers_init( void );
uint8_t registers_host_read( uint8_t idx );
void registers_host_write( uint8_t idx, uint8_t data );
uint8_t registers_get( uint8_t index );

#endif  // __REGISTERS_H__

