#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "registers.h"
#include "board.h"


static uint8_t registers[ NUM_REGISTERS ];

// Internal interface
void registers_set_mask( uint8_t index, uint8_t mask )
{
    registers[ index ] |= mask;
}


void registers_clear_mask( uint8_t index, uint8_t mask )
{
    registers[ index ] &= ~mask;
}


uint8_t registers_get( uint8_t index )
{
    return registers[ index ];
}


void registers_set( uint8_t index, uint8_t value )
{
    registers[ index ] = value;
}


// Host interface
uint8_t registers_host_read( uint8_t index )
{
    return registers[ index ];
}


void registers_host_write( uint8_t index, uint8_t data )
{
    registers[ index ] = data;
    
    switch ( index )
    {
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
        default:
        {
            break;
        }
    }
}
//

void registers_init( void )
{
    registers[ REG_CONTROL ]        = CONTROL_CE;
    registers[ REG_START_ENABLE ]   = ( START_BUTTON | START_EXTERNAL | START_PWRGOOD );
    registers[ REG_RESTART_HOURS ]   = 0;
    registers[ REG_RESTART_MINUTES ] = 0;
    registers[ REG_RESTART_SECONDS ] = 0;
}

