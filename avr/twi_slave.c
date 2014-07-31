#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "registers.h"
#include "twi_slave.h"


uint8_t data_count = 0;
uint8_t reg_index = 0;

ISR( TWI_vect )
{
    uint8_t data;
    uint8_t status = TWSR & 0xFC;

    switch ( status )
    {
        case 0x60:  // SLA+W
        case 0xA0:  // Stop or repeated start
        {
            data_count = 0;
            break;
        }
        
        case 0xA8:  // SLA+R
        case 0xB8:  // Data sent + ACK
        {
            TWDR = registers_host_read( reg_index++ );
            
            if ( reg_index >= NUM_REGISTERS )
            {
                reg_index = 0;
            }
                
            break;
        }
        
        case 0x80:  // data RX
        {
            data = TWDR;
            TWCR |= ( 1 << TWINT );
            
            if ( data_count == 0 )
            {
                reg_index = data;
            }
            else
            {
                registers_host_write( reg_index++, data );
            }
            
            if ( reg_index >= NUM_REGISTERS )
            {
                reg_index = 0;
            }
            
            data_count++;
            return;
        }
        
        case 0xC0:  // data TX + NAK
        case 0xC8:  // last data (TWEA=0) + ACK
        {
            break;
        }
        
        default:
        {
            break;
        }
    }
    
    // Last thing is to clear the INT flag
    TWCR |= ( 1 << TWINT );
}


void twi_slave_init( void )
{
    uint8_t i;
    
    i = registers_get( REG_I2C_ADDRESS );
    if ( i & 0x80 )
    {
        i = TWI_SLAVE_ADDRESS;
    }
    
    TWAR = ( i << 1 );
    TWCR = ( 1 << TWEA ) | ( 1 << TWEN ) | ( 1 << TWIE );
}


void twi_slave_stop( void )
{
    TWCR = 0;
}

