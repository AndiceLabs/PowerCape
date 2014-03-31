#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "registers.h"
#include "board.h"


extern volatile uint32_t seconds;
static uint8_t registers[ NUM_REGISTERS ];

// Internal interface
inline void registers_set_mask( uint8_t index, uint8_t mask )
{
    registers[ index ] |= mask;
}


inline void registers_clear_mask( uint8_t index, uint8_t mask )
{
    registers[ index ] &= ~mask;
}


inline uint8_t registers_get( uint8_t index )
{
    return registers[ index ];
}


inline void registers_set( uint8_t index, uint8_t value )
{
    registers[ index ] = value;
}


// Host interface
uint8_t registers_host_read( uint8_t index )
{
    switch ( index )
    {
        case REG_STATUS:
        {
            // Update pgood status
            board_pgood();
            break;
        }
        case REG_SECONDS_0:
        {
            registers[ REG_SECONDS_0 ] = ( uint8_t )( seconds & 0xFF );
            break;
        }
        case REG_SECONDS_1:
        {
            registers[ REG_SECONDS_1 ] = ( uint8_t )( ( seconds & 0xFF00 ) >> 8 );
            break;
        }
        case REG_SECONDS_2:
        {
            registers[ REG_SECONDS_2 ] = ( uint8_t )( ( seconds & 0xFF0000 ) >> 16 );
            break;
        }
        case REG_SECONDS_3:
        {
            registers[ REG_SECONDS_3 ] = ( uint8_t )( ( seconds & 0xFF000000 ) >> 24 );
            break;
        }
    }
    
    return registers[ index ];
}


void registers_host_write( uint8_t index, uint8_t data )
{
    registers[ index ] = data;
    
    switch ( index )
    {
        case REG_OSCCAL:
        {
            OSCCAL = data;
            break;
        }
        case REG_CONTROL:
        {
            if ( data & CONTROL_CE )
            {
                board_ce( 1 );
            }
            else
            {
                board_ce( 0 );
            }
            
            if ( data & CONTROL_LED0 )
            {
                board_led_on( 0 );
            }
            else
            {
                board_led_off( 0 );
            }

            if ( data & CONTROL_LED1 )
            {
                board_led_on( 1 );
            }
            else
            {
                board_led_off( 1 );
            }
            
            break;
        }
        
        case REG_RESTART_HOURS:
        case REG_RESTART_MINUTES:
        case REG_RESTART_SECONDS:
        {
            registers_set_mask( REG_START_ENABLE, START_TIMEOUT );
            break;
        }
        
        default:
        {
            break;
        }
    }
}


void registers_init( void )
{
    registers[ REG_CONTROL ]        = CONTROL_CE;
    registers[ REG_START_ENABLE ]   = START_ALL;
    registers[ REG_RESTART_HOURS ]   = 0;
    registers[ REG_RESTART_MINUTES ] = 0;
    registers[ REG_RESTART_SECONDS ] = 0;
}

