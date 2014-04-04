#ifndef __REGISTERS_H__
#define __REGISTERS_H__

enum registers_type {
    REG_MCUSR,                  // 0 - AVR register
    REG_OSCCAL,                 // 1 - AVR register
    REG_STATUS,                 // 2 
    REG_CONTROL,                // 3
    REG_START_ENABLE,           // 4
    REG_START_REASON,           // 5
    REG_RESTART_HOURS,          // 6    Countdown hours
    REG_RESTART_MINUTES,        // 7    Countdown minutes
    REG_RESTART_SECONDS,        // 8    Countdown seconds
    REG_SECONDS_0,              // 9    Uptime counter/clock
    REG_SECONDS_1,              // 10   "
    REG_SECONDS_2,              // 11   "
    REG_SECONDS_3,              // 12   "
        
    NUM_REGISTERS
};


// STATUS register bits
#define STATUS_POWER_GOOD       0x01

// CONTROL register bits
#define CONTROL_CE              0x01
#define CONTROL_LED0            0x02
#define CONTROL_LED1            0x04
#define CONTROL_BOOTLOAD        0x80

// START enable and reason register bits
#define START_BUTTON            0x01
#define START_EXTERNAL          0x02
#define START_PWRGOOD           0x04
#define START_TIMEOUT           0x08
#define START_ALL               0x0F


void registers_init( void );
inline void registers_set_mask( uint8_t index, uint8_t mask );
inline void registers_clear_mask( uint8_t index, uint8_t mask );
inline uint8_t registers_get( uint8_t index );
inline void registers_set( uint8_t idx, uint8_t data );
uint8_t registers_host_read( uint8_t idx );
void registers_host_write( uint8_t idx, uint8_t data );


#endif  // __REGISTERS_H__

