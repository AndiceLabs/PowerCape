#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "registers.h"


static uint8_t registers[ NUM_REGISTERS ];

// Internal interface
void registers_set_mask( uint8_t index, uint8_t mask )
{
}


void registers_clear_mask( uint8_t index, uint8_t mask )
{
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
    registers[ REG_FEATURE_MASK ]   = 0x69;
}

