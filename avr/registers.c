#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "registers.h"


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


// Host interface
uint8_t registers_host_read( uint8_t index )
{
    return registers[ index ];
}


void registers_host_write( uint8_t index, uint8_t data )
{
    registers[ index ] = data;
}
//

void registers_init( void )
{
    registers[ REG_FEATURE_MASK ]    = 0x69;
    registers[ REG_START_ENABLE ] = ( START_BUTTON | START_EXTERNAL | START_PWRGOOD );
    registers[ REG_RESTART_HOURS ]   = 0;
    registers[ REG_RESTART_MINUTES ] = 0;
    registers[ REG_RESTART_SECONDS ] = 0;
}

