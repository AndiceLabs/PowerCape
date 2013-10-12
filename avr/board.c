#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "registers.h"
#include "board.h"


extern void button_press( void );
extern void countdown_complete( void );

volatile uint16_t system_ticks;
volatile uint8_t countdown_hours, countdown_minutes, countdown_seconds;


uint8_t board_begin_countdown( void )
{
    countdown_hours   = registers_get( REG_RESTART_HOURS );
    countdown_minutes = registers_get( REG_RESTART_MINUTES );
    countdown_seconds = registers_get( REG_RESTART_SECONDS );
    
    if ( ( countdown_hours == 0 ) && ( countdown_minutes == 0 ) &&
         ( countdown_seconds == 0 ) )
    {
        return 0;
    }
    return 1;
}


void board_power_on( void )
{
    PORTD |= PIN_ENABLE;
}


void board_power_off( void )
{
    PORTD &= ~PIN_ENABLE;
}


uint8_t board_powered( void )
{
    return ( PORTD & PIN_ENABLE ) ? 1 : 0;
}


uint8_t board_3v3( void )
{
    return ( PIND & PIN_3V3 ) ? 1 : 0;
}


void board_gpio_config( void )
{
    // Enable pull-ups on input pins to keep unconnected 
    // ones from floating
    DDRB = 0;
    PORTB = 0xFF;               // engage all PORTB pull-ups

    DDRD = PIN_ENABLE;          // INT0 is system power enable
    PORTD = ~( PIN_ENABLE | PIN_3V3 );      // INT1 detects BB system voltage         
    
    // Pin change interrupt0
    PCMSK0 = ( 1 << PCINT0 );   // PCINT0 = PB0 (button)
    PCMSK2 = ( 1 << PCINT23 );  // OPTO = PD7 = PCINT23
    PCICR  = ( ( 1 << PCIE0 ) | ( 1 << PCIE2 ) );
}


// Button press
ISR( PCINT0_vect )
{
    button_press();
}


// OPTO
ISR( PCINT2_vect )
{
    button_press();
}


ISR( TIMER2_OVF_vect )
{
    if ( countdown_seconds > 0 )
    {
        countdown_seconds--;
    }
    else if ( countdown_minutes > 0 )
    {
        countdown_minutes--;
        countdown_seconds = 59;
    }
    else if ( countdown_hours > 0 )
    {
        countdown_hours--;
        countdown_minutes = 59;
    }
    else
    {
        countdown_complete();
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



