#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "board.h"
#include "registers.h"
#include "twi_slave.h"

uint8_t mcusr __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr( void )
{
    mcusr = MCUSR;
    MCUSR = 0;
    wdt_disable();
}


enum state_type {
    STATE_INIT,
    STATE_OFF,
    STATE_COUNTDOWN,
    STATE_POWER_ON,
    STATE_ON,
};

volatile uint8_t power_state = STATE_INIT;


void countdown_complete( void )
{
    if ( power_state == STATE_COUNTDOWN )
    {
        power_state = STATE_POWER_ON;
    }
}


void button_press( void )
{
    if ( ( power_state == STATE_OFF ) || ( power_state == STATE_COUNTDOWN ) )
    {
        power_state = STATE_POWER_ON;
    }
}


void state_machine( void )
{
    switch( power_state )
    {
        default:
        case STATE_INIT:
        {
            if ( board_begin_countdown() == 0 )
            {
                power_state = STATE_OFF;
            }
            else
            {
                power_state = STATE_COUNTDOWN;
            }
            break;
        }
        
        case STATE_COUNTDOWN:
        case STATE_OFF:
        {
            // Enable external events
            break;
        }
        
        case STATE_POWER_ON:
        {
            board_power_on();
            if ( board_3v3() != 0 )
            {
                power_state = STATE_ON;
            }
            break;
        }
        
        case STATE_ON:
        {
            // Check for countdown value
            if ( board_3v3() == 0 )
            {
                board_power_off();
                power_state = STATE_INIT;
            }
            break;
        }
    }
}


int main( void )
{
    // Platform setup
    board_init();
    registers_init();
    twi_slave_init();
    
    set_sleep_mode( SLEEP_MODE_PWR_SAVE );
    sei();
    
    // Main loop
    while ( 1 ) 
    {
        state_machine();
        if ( power_state != STATE_ON )
        {
            sleep_enable();
            sleep_cpu();
            sleep_disable();
        }
    }
}

