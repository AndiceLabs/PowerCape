#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "board.h"
#include "eeprom.h"
#include "registers.h"
#include "twi_slave.h"


extern volatile uint16_t system_ticks;
volatile uint8_t rebootflag = 0;
volatile uint8_t activity_watchdog;

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
    STATE_WDT_POWER,
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


void watchdog_reset( void )
{
    // Make sure there is no start reason
    registers_set( REG_START_REASON, 0 );
    board_hold_reset();
    _delay_ms( 250 );
    board_release_reset();
}


void watchdog_check( void )
{
    uint8_t i;
    
    // Check reset watchdog
    i = registers_get( REG_WDT_RESET );
    if ( i != 0 )
    {
        i -= 1;
        registers_set( REG_WDT_RESET, i );
        if ( i == 0 )
        {
            watchdog_reset();
            registers_set( REG_WDT_POWER, 0 );
            registers_set( REG_WDT_STOP, 0 );
        }
    }
    
    // Check power-cycle watchdog
    i = registers_get( REG_WDT_POWER );
    if ( i != 0 )
    {
        i -= 1;
        registers_set( REG_WDT_POWER, i );
        if ( i == 0 )
        {
            power_state = STATE_WDT_POWER;
        }
    }

    // Check power-down watchdog
    i = registers_get( REG_WDT_STOP );
    if ( i != 0 )
    {
        i -= 1;
        registers_set( REG_WDT_STOP, i );
        if ( i == 0 )
        {
            power_state = STATE_POWER_DOWN;
        }
    }
    
    // Check start-up activity watchdog
    if ( activity_watchdog != 0 )
    {
        activity_watchdog -= 1;
        if ( activity_watchdog == 0 )
        {
            power_state = STATE_WDT_POWER;
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
                power_state = STATE_CHECK_3V;
            }
            else
            {
                power_state = STATE_POWER_DOWN;
            }
            break;
        }
        
        case STATE_CLEAR_MASK:
        {
            board_enable_interrupt( registers_get( REG_START_ENABLE ) );
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
                while ( ASSR & ( 1 << TCR2AUB ) ) { /* wait */ }
                sleep_enable();
                sleep_cpu();
                sleep_disable();
                TCCR2A = 0;
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
                while ( ASSR & ( 1 << TCR2AUB ) ) { /* wait */ }
                sleep_enable();
                sleep_cpu();
                sleep_disable();
                TCCR2A = 0;
            }
            break;
        }
        
        case STATE_POWER_UP:
        {
            retries--;

            registers_set( REG_WDT_RESET, 0 );
            registers_set( REG_WDT_POWER, 0 );
            registers_set( REG_WDT_STOP, 0 );

            board_hold_reset();
            board_power_on();
            _delay_ms( 250 );
            board_release_reset();
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
                activity_watchdog = registers_get( REG_WDT_START );
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
        
        case STATE_WDT_POWER:
        {
            twi_slave_stop();
            board_power_off();
            retries = 3;
            power_state = STATE_POWER_UP;
            break;
        }
    }
}


int main( void )
{
    uint8_t oscval;
    uint16_t last_tick = 0;
    
    // Make sure DIV8 is selected
#if 0 // PI test: staying at 8MHz fixes bus timeouts
    if ( CLKPR != 0x03 )    // Div8
    {
        CLKPR = ( 1 << CLKPCE );
        CLKPR = 0x03;
    }
#endif
    
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

    board_set_charge_current( registers_get( REG_I2C_ICHARGE ) );
    board_set_charge_timer( registers_get( REG_I2C_TCHARGE ) );
    
    // Main loop
    while ( 1 )
    {
        wdt_reset();
        
        // System ticks are seconds
        if ( last_tick != system_ticks )
        {
            last_tick = system_ticks;
            state_machine();
            if ( power_state == STATE_ON )
            {
                watchdog_check();                
            }
        }
        
        // Bootloader entry
        if ( rebootflag != 0 )
        {
            twi_slave_stop();
            board_stop();
            eeprom_set_bootloader_flag();
            cli();
            wdt_enable( WDTO_30MS );
            while ( 1 );
        }
        
        // Register handling
        if ( registers_get( REG_OSCCAL ) != oscval )
        {
            oscval = registers_get( REG_OSCCAL );
            eeprom_set_calibration_value( oscval );
            OSCCAL = oscval;
        }
    }
}

