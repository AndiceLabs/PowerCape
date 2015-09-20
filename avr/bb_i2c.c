#include <stdlib.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "board.h"
#include "bb_i2c.h"

#define BANG_DELAY  16

#if 0
#define SCL_LOW     ( *scl_ddr |= scl_mask )
#define SCL_HIGH    ( *scl_ddr &= ~scl_mask )
#define SDA_LOW     ( *sda_ddr |= sda_mask )
#define SDA_HIGH    ( *sda_ddr &= ~sda_mask )

volatile uint8_t *scl_ddr;
uint8_t  scl_mask;
volatile uint8_t *sda_ddr;
uint8_t  sda_mask;
#else
#define SCL_LOW     ( DDRC |= PIN_SCL )
#define SCL_HIGH    ( DDRC &= ~PIN_SCL )
#define SDA_LOW     ( DDRC |= PIN_SDA )
#define SDA_HIGH    ( DDRC &= ~PIN_SDA )
#endif

#define NOP         asm volatile( "nop\n\t" )



/* Going to need external pull-ups to use GPIO in open-collector mode */
void bb_i2c_init( void )
{
    // Set pins to tri-state
    DDRC &= ~PIN_SCL;
    DDRC &= ~PIN_SDA;
    
    // Pre-set output to low (this also disables the internal pull-up)
    PORTC &= ~PIN_SCL;
    PORTC &= ~PIN_SDA;
}


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


void bb_start( void )
{
    // Idle bus == both lines high
    SDA_LOW;
    delay_loop();
    SCL_LOW;    
    delay_loop();
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
        delay_loop();
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
        if ( PINC & PIN_SDA )
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
    i = ~( PINC & PIN_SDA );
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

