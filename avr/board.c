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
    DDRC = 0;
    PORTC = ~( PIN_SDA | PIN_SCL );

    DDRD = PIN_ENABLE;          // system power enable
    PORTD = ~( PIN_ENABLE | PIN_3V3 );      // INT1 detects BB system voltage         
    
    // Pin change interrupt0
    PCMSK1 = ( PIN_PGOOD );   // REB: TODO
    PCMSK2 = ( PIN_OPTO | PIN_BUTTON );  
    PCICR  = ( ( 1 << PCIE2 ) | ( 1 << PCIE1 ) );
}


// Power Good
ISR( PCINT1_vect )
{
    if ( ( PINC & PIN_PGOOD ) == 0 )
    {
        power_event( START_PWRGOOD );
    }
}


// OPTO and Button
ISR( PCINT2_vect )
{
    if ( ( PIND & PIN_BUTTON ) == 0 )
    {
        power_event( START_BUTTON );
    }

    if ( ( PIND & PIN_OPTO ) == 0 )
    {
        power_event( START_EXTERNAL );
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



