#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "registers.h"
#include "board.h"


extern void power_event( uint8_t reason );

volatile uint16_t system_ticks;
volatile uint32_t countdown_seconds;
volatile uint32_t uptime;


uint8_t board_begin_countdown( void )
{
    countdown_seconds = registers_get( REG_RESTART_HOURS ) * 3600;
    countdown_seconds += registers_get( REG_RESTART_MINUTES ) * 60;
    countdown_seconds += registers_get( REG_RESTART_SECONDS );
    
    if ( countdown_seconds == 0 )
    {
        return 0;
    }
    return 1;
}


void board_power_on( void )
{
    PORTD |= PIN_D;
    asm volatile( "nop\n\t" );
    PORTD |= PIN_CP;
    asm volatile( "nop\n\t" );
    PORTD &= ~PIN_CP;
    PORTD &= ~PIN_D;
}


void board_power_off( void )
{
    PORTD &= ~PIN_D;
    asm volatile( "nop\n\t" );
    PORTD |= PIN_CP;
    asm volatile( "nop\n\t" );
    PORTD &= ~PIN_CP;
}


void board_led_on( uint8_t led )
{
    if ( led == 0 )
    {
        PORTB |= PIN_LED0;
    }
    else
    {
        PORTB |= PIN_LED1;
    }
}


void board_led_off( uint8_t led )
{
    if ( led == 0 )
    {
        PORTB &= ~PIN_LED0;
    }
    else
    {
        PORTB &= ~PIN_LED1;
    }
}


void board_ce( uint8_t enable )
{
    if ( enable )
    {
        DDRC &= ~PIN_CE;
    }
    else
    {
        DDRC |= PIN_CE;
    }
}


uint8_t board_3v3( void )
{
    return ( PIND & PIN_DETECT );
}


void board_gpio_config( void )
{
    // Enable pull-ups on input pins to keep unconnected 
    // ones from floating
    PORTB = ~( PIN_LED1 | PIN_LED0 | PIN_XTAL1 | PIN_XTAL2 );   // engage all PORTB pull-ups
    DDRB = ( PIN_LED1 | PIN_LED0 );

    PORTC = ~( PIN_SDA | PIN_SCL | PIN_CE );
    DDRC = 0;

    PORTD = ~( PIN_CP | PIN_D | PIN_DETECT | PIN_TXD | PIN_RXD );
    DDRD = ( PIN_CP | PIN_D );
    
    // Pin change interrupt0
    PCMSK0 = ( PIN_OPTO );
    PCMSK1 = ( PIN_PGOOD ); 
    PCMSK2 = ( PIN_BUTTON );  
    PCICR  = ( 1 << PCIE2 ) | ( 1 << PCIE1 ) | ( 1 << PCIE0 );
}


// OPTO
ISR( PCINT0_vect )
{
    if ( ( PINB & PIN_OPTO ) == 0 )
    {
        power_event( START_EXTERNAL );
    }
}


// Power Good
ISR( PCINT1_vect )
{
    if ( ( PINC & PIN_PGOOD ) == 0 )
    {
        power_event( START_PWRGOOD );
    }
}


// Button
ISR( PCINT2_vect )
{
    if ( ( PIND & PIN_BUTTON ) == 0 )
    {
        power_event( START_BUTTON );
    }
}


ISR( TIMER2_OVF_vect )
{
    uptime++;
    countdown_seconds--;

    if ( countdown_seconds == 0 )
    {
        power_event( START_TIMEOUT );
    }
}


void timer2_init( void )
{
    PRR &= ~( 1 << PRTIM2 );    // is this necessary with async mode?
    ASSR = ( 1 << AS2 );        // external crystal
    TCCR2B = 0;
    TCCR2A = 0;
    TCNT2 = 0;
    TCCR2B = ( 1 << CS22 ) | ( 1 << CS20 );    // clk/128
    TIMSK2 = ( 1 << TOIE2 );
}


void board_init( void )
{
    board_gpio_config();
    PRR |= ( ( 1 << PRTIM0 ) | ( 1 << PRTIM1 ) );
    timer2_init();    
    PRR |= ( ( 1 << PRSPI ) | ( 1 << PRUSART0 ) | ( 1 << PRADC ) );
}



