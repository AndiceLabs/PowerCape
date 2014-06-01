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
#include "eeprom.h"
#include "registers.h"
#include "twi_slave.h"


extern volatile uint16_t system_ticks;
volatile uint8_t rebootflag = 0;

uint8_t mcusr __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr( void )
{
    mcusr = MCUSR;
    MCUSR = 0;
    wdt_enable( WDTO_2S );
}


enum state_type {
    STATE_INIT,
    STATE_CLEAR_MASK,
    STATE_OFF_NO_PGOOD,
    STATE_OFF_WITH_PGOOD,
    STATE_POWER_UP,
    STATE_CHECK_3V,
    STATE_ON,
    STATE_POWER_DOWN,
};

volatile uint8_t power_state = STATE_INIT;
uint8_t retries = 0;


void power_down( void )
{
    if ( power_state == STATE_ON )
    {
        power_state = STATE_POWER_DOWN;
    }
}


void power_event( uint8_t reason )
{
    if ( ( power_state == STATE_OFF_NO_PGOOD ) || ( power_state == STATE_OFF_WITH_PGOOD ) )
    {
        if ( registers_get( REG_START_ENABLE ) & reason )
        {
            retries = 3;
            power_state = STATE_POWER_UP;
            registers_set_mask( REG_START_REASON, reason );
        }
    }
}


void state_machine( void )
{
    switch( power_state )
    {
        default:
        case STATE_INIT:
        {
            if ( board_3v3() )
            {
                retries = 3;
                power_state = STATE_POWER_UP;
            }
            else
            {
                power_state = STATE_POWER_DOWN;
            }
            break;
        }
        
        case STATE_CLEAR_MASK:
        {
            uint8_t reg = registers_get( REG_START_ENABLE ); 
            
            board_enable_interrupt( reg );
            board_begin_countdown();
            
            if ( board_pgood() )
            {
                power_state = STATE_OFF_WITH_PGOOD;
            }
            else
            {
                power_state = STATE_OFF_NO_PGOOD;
            }
            
            registers_clear_mask( REG_START_REASON, 0xFF );

            break;
        }
        
        case STATE_OFF_NO_PGOOD:
        {
            if ( board_pgood() )
            {
                power_state = STATE_OFF_WITH_PGOOD;
                power_event( START_PWRGOOD );
            }
            else
            {
                sleep_enable();
                sleep_cpu();
                sleep_disable();
            }
            break;
        }
        
        case STATE_OFF_WITH_PGOOD:
        {
            if ( !board_pgood() )
            {
                power_state = STATE_OFF_NO_PGOOD;
            }
            else
            {
                sleep_enable();
                sleep_cpu();
                sleep_disable();
            }
            break;
        }
        
        case STATE_POWER_UP:
        {
            retries--;
            board_power_on();
            power_state = STATE_CHECK_3V;            
            break;
        }
        
        case STATE_CHECK_3V:
        {
            if ( board_3v3() )
            {
                power_state = STATE_ON;
                twi_slave_init();
                board_disable_interrupt( START_ALL );
            }
            else
            {
                board_power_off();
                if ( retries > 0 )
                {
                    power_state = STATE_POWER_UP;
                }
                else
                {
                    power_state = STATE_POWER_DOWN;
                }
            }
            break;
        }
        
        case STATE_ON:
        {
            if ( board_3v3() == 0 )
            {
                power_state = STATE_POWER_DOWN;
            }
            break;
        }
        
        case STATE_POWER_DOWN:
        {
            twi_slave_stop();
            board_power_off();
            power_state = STATE_CLEAR_MASK;
            break;
        }
    }
}


int main( void )
{
    uint8_t oscval;
    uint16_t last_tick = 0;
    
    // Make sure DIV8 is selected
    if ( CLKPR != 0x03 )    // Div8
    {
        CLKPR = ( 1 << CLKPCE );
        CLKPR = 0x03;
    }
    
    // Platform setup
    board_init();
    registers_init();
    registers_set( REG_MCUSR, mcusr );
    
    oscval = eeprom_get_calibration_value();
    if ( oscval != 0xFF )
    {
        OSCCAL = oscval;
    }
    else
    {
        oscval = OSCCAL;
    }
    registers_set( REG_OSCCAL, oscval );
    
    set_sleep_mode( SLEEP_MODE_PWR_SAVE );
    sei();

    // Main loop
    while ( 1 )
    {
        wdt_reset();
        
        if ( last_tick != system_ticks )
        {
            last_tick = system_ticks;
            state_machine();
        }

        if ( rebootflag != 0 )
        {
            twi_slave_stop();
            board_stop();
            eeprom_set_bootloader_flag();
            cli();
            wdt_enable( WDTO_30MS );
            while ( 1 );
        }
        
        if ( registers_get( REG_OSCCAL ) != oscval )
        {
            oscval = registers_get( REG_OSCCAL );
            eeprom_set_calibration_value( oscval );
            OSCCAL = oscval;
        }
    }
}

