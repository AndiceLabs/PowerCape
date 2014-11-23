#include <stdlib.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "board.h"
#include "bb_i2c.h"

#define BANG_DELAY  16

#define SCL_LOW     DDRD |= PIN_BB_SCL
#define SCL_HIGH    DDRD &= ~PIN_BB_SCL
#define SDA_LOW     DDRD |= PIN_BB_SDA
#define SDA_HIGH    DDRD &= ~PIN_BB_SDA

#define NOP         asm volatile( "nop\n\t" )


static inline void delay_loop( void )
{
    uint8_t i;

    for ( i = 0; i < BANG_DELAY; i++ )
    {
        NOP;
    }
}


static inline void delay_loop2( void )
{
    uint8_t i;

    for ( i = 0; i < BANG_DELAY/2; i++ )
    {
        NOP;
    }
}


/* Going to need external pull-ups to use GPIO in open-collector mode */
void bb_i2c_init( void )
{
    // Set pins to tri-state
    DDRD &= ~PIN_BB_SCL;
    DDRD &= ~PIN_BB_SDA;
    
    // Pre-set output to low (this also disables the internal pull-up)
    PORTD &= ~PIN_BB_SCL;
    PORTD &= ~PIN_BB_SDA;
}


void bb_start( void )
{
    // Idle bus == both lines high
    SDA_LOW;
    delay_loop();
    SCL_LOW;    
}


void bb_stop( void )
{
    SDA_LOW;
    delay_loop();
    SCL_HIGH;
    delay_loop();
    SDA_HIGH;    
}


void bb_out( uint8_t b )
{
    uint8_t i;
    
    for ( i = 0; i < 8; i++ )
    {
        if ( b & 0x80 )
        {
            SDA_HIGH;
        }
        else
        {
            SDA_LOW;
        }
        SCL_HIGH;
        delay_loop();
        SCL_LOW;
        b <<= 1;
    }
    // Release SDA
    SDA_HIGH;
}


uint8_t bb_in( void )
{
    uint8_t i, b = 0;
    
    SCL_LOW;
    for ( i = 0; i < 8; i++ )
    {
        delay_loop2();
        SCL_HIGH;
        delay_loop2();
        if ( PIND & PIN_BB_SDA )
        {
            b |= 0x01;
        }
        delay_loop2();
        SCL_LOW;
        if ( i < 7 )
        {
            b <<= 1;
        }
        delay_loop2();
    }
    return b;
}


uint8_t get_ack( void )
{
    uint8_t i;
    
    SDA_HIGH;
    SCL_HIGH;
    delay_loop2();
    i = ~( PIND & PIN_BB_SDA );
    delay_loop2();
    SCL_LOW;
    
    return i;
}


void send_ack( void )
{
    SDA_LOW;
    NOP; NOP; NOP;
    SCL_HIGH;
    delay_loop();
    SCL_LOW;
    NOP; NOP; NOP;
    SDA_HIGH;
}


void send_nak( void )
{
    SDA_HIGH;
    NOP; NOP; NOP;
    SCL_HIGH;
    delay_loop();
    SCL_LOW;
    NOP; NOP; NOP;
}


uint8_t bb_i2c_read( uint8_t addr, uint8_t *buf, uint8_t len )
{
    uint8_t rc = 0;
    
    bb_start();

    bb_out( addr | 0x01 ); // address + R
    if ( get_ack() )
    {
        while ( len > 0 )
        {
            *buf++ = bb_in();
                
            len--;
            if ( len > 0 )
            {
                send_ack();
            }
            else
            {
                send_nak();
            }
        }
    }
    else
    {
        rc = 1;
    }
    
    bb_stop();
    
    return rc;
}


uint8_t bb_i2c_write( uint8_t addr, uint8_t *buf, uint8_t len )
{
    uint8_t rc = 0;

    bb_start();
    
    bb_out( addr );    // address + W
    if ( get_ack() )
    {
        while ( len > 0 )
        {
            bb_out( *buf++ );
            
            if ( get_ack() == 0 )
            {
                rc = 1;
                break;
            }
            
            len--;
        }
    }
    else 
    {
        rc = 1;
    }

    bb_stop();

    return rc;
}

