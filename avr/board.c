#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "registers.h"
#include "bb_i2c.h"
#include "board.h"


extern void power_down( void );
extern void power_event( uint8_t reason );

volatile uint16_t system_ticks;
volatile uint32_t countdown;
volatile uint32_t seconds;


uint8_t board_begin_countdown( void )
{
    countdown = (uint32_t)registers_get( REG_RESTART_HOURS ) * 3600;
    countdown += (uint32_t)registers_get( REG_RESTART_MINUTES ) * 60;
    countdown += (uint32_t)registers_get( REG_RESTART_SECONDS );
    
    if ( countdown == 0 )
    {
        registers_clear_mask( REG_START_ENABLE, START_TIMEOUT );
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
#ifdef DEBUG
    board_led_on( 1 );
#endif
}


void board_power_off( void )
{
    PORTD &= ~PIN_D;
    asm volatile( "nop\n\t" );
    PORTD |= PIN_CP;
    asm volatile( "nop\n\t" );
    PORTD &= ~PIN_CP;
#ifdef DEBUG
    board_led_off( 1 );
#endif
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


void board_hold_reset( void )
{
    if ( registers_get( REG_BOARD_TYPE ) == BOARD_TYPE_BONE )
    {
        PORTD &= ~PIN_DETECT;
        DDRD |= PIN_DETECT;
    }
}


void board_release_reset( void )
{
    if ( registers_get( REG_BOARD_TYPE ) == BOARD_TYPE_BONE )
    {
        DDRD &= ~PIN_DETECT;
    }
}


uint8_t board_pgood( void )
{
    uint8_t rc = 0;
    
    if ( PINC & PIN_PGOOD )
    {
        registers_clear_mask( REG_STATUS, STATUS_POWER_GOOD );
    }
    else
    {
        registers_set_mask( REG_STATUS, STATUS_POWER_GOOD );
        rc = 1;
    }
    
    return rc;
}


void board_enable_pgood_irq( void )
{
    // Enable PGOOD interrupt
    PCMSK1 |= ( PIN_PGOOD );
    PCICR  |= ( 1 << PCIE1 );    
}


void board_enable_interrupt( uint8_t mask )
{
    if ( mask & START_EXTERNAL )
    {
        PCMSK0 |= ( PIN_OPTO );
        PCICR  |= ( 1 << PCIE0 );
    }

    if ( mask & START_BUTTON )
    {
        PCMSK2 |= ( PIN_BUTTON );
        PCICR  |= ( 1 << PCIE2 );
    }
}


void board_disable_interrupt( uint8_t mask )
{
    if ( mask & START_EXTERNAL )
    {
        PCMSK0 &= ~( PIN_OPTO );
        PCICR  &= ~( 1 << PCIE0 );
    }

    if ( mask & START_BUTTON )
    {
        PCMSK2 &= ~( PIN_BUTTON );
        PCICR  &= ~( 1 << PCIE2 );
    }
}


void board_set_charge_current( uint8_t thirds )
{
    uint8_t pins;
    
    if ( ( registers_get( REG_BOARD_REV ) == 'A' ) &&
         ( registers_get( REG_BOARD_STEP ) >= '2' ) )
    {        
        DDRC &= ~( PIN_ISET2 | PIN_ISET3 );
        PORTC &= ~( PIN_ISET2 | PIN_ISET3 );
        
        switch ( thirds )
        {
            case 3:  pins = ( PIN_ISET2 | PIN_ISET3 ); break;
            case 2:  pins = PIN_ISET2; break;
            case 1:
            default: pins = PIN_ISET3; break;
            case 0:  pins = 0; break;
        }
        DDRC |= pins;
    }
}


uint8_t wiper_value[ 11 ] = {
    0,      // +0
    13,     // +10k
    26,     // +20k
    38,     // +30k
    51,     // +40k
    64,     // +50k
    76,     // +60k
    89,     // +70k
    102,    // +80k
    114,    // +90k
    127,    // +100k (max)
};


void board_set_charge_timer( uint8_t hours )
{
    uint8_t b;

    if ( ( registers_get( REG_BOARD_REV ) == 'A' ) &&
         ( registers_get( REG_BOARD_STEP ) >= '2' ) )
    {    
        if ( hours > 10 ) 
        {
            hours = 10;
        }
        
        b = wiper_value[ hours - 3 ];
        if ( bb_i2c_write( MCP_ADDR, &b, 1 ) )
        {
            // indicate error
            registers_set( REG_I2C_TCHARGE, 0xEE );
        }
    }
}


void board_gpio_config( void )
{
    // Enable pull-ups on input pins to keep unconnected 
    // ones from floating
    PORTB = ~( PIN_LED1 | PIN_LED0 | PIN_XTAL1 | PIN_XTAL2 );   // engage all PORTB pull-ups
    DDRB  = ( PIN_LED1 | PIN_LED0 );

    PORTC  = ~( PIN_SDA | PIN_SCL | PIN_CE | PIN_ISET2 | PIN_ISET3 );
    DDRC   = PIN_ISET3;

    PORTD = ~( PIN_CP | PIN_D | PIN_DETECT | PIN_BB_SCL | PIN_BB_SDA );
    DDRD  = ( PIN_CP | PIN_D );
    
    board_enable_pgood_irq();   
}


// OPTO
ISR( PCINT0_vect, ISR_BLOCK )
{
    PCMSK0 &= ~PIN_OPTO;

    if ( ( PINB & PIN_OPTO ) == 0 )
    {
        power_event( START_EXTERNAL );
    }
}


// Power Good oscillation fix
ISR( PCINT1_vect, ISR_BLOCK )
{
    if ( ( PINC & PIN_PGOOD ) == 0 )
    {
        PCMSK1 &= ~PIN_PGOOD;
        board_ce( 0 );
    }
}


// Button
ISR( PCINT2_vect, ISR_BLOCK )
{
    if ( ( PIND & PIN_BUTTON ) == 0 )
    {
        PCMSK2 &= ~PIN_BUTTON;
        power_event( START_BUTTON );
    }
}


ISR( TIMER2_OVF_vect, ISR_BLOCK )
{
    static uint8_t button_hold_count = 0;
    
    // Handle RTC
    system_ticks++;
#ifdef DEBUG
    PORTB ^= PIN_LED0;
#endif
    seconds++;
    
    // Check for startup conditions
    if ( countdown != 0 )
    {
        countdown--;
        if ( countdown == 0 )
        {
            power_event( START_TIMEOUT );
        }
    }
    
    // Forced power-off check
    if ( ( PIND & PIN_BUTTON ) == 0 )
    {
        button_hold_count++;
        if ( button_hold_count == 5 )
        {
            power_down();
        }
    }
    else
    {
        button_hold_count = 0;
    }
}


void timer2_init( void )
{
    PRR &= ~( 1 << PRTIM2 );    // is this necessary with async mode?
    ASSR = ( 1 << AS2 );        // external crystal
    TCCR2B = 0;
    TCCR2A = 0;
    TCNT2 = 0;
    TCCR2B = ( 1 << CS22 ) | ( 1 << CS20 );    // clk/128 (1s)
    TIMSK2 = ( 1 << TOIE2 );
}


void board_init( void )
{
    board_gpio_config();
    PRR = ( ( 1 << PRTIM0 ) | ( 1 << PRTIM1 ) | ( 1 << PRSPI ) | ( 1 << PRUSART0 ) | ( 1 << PRADC ) );
    timer2_init();
    bb_i2c_init();
}


void board_stop( void )
{
    TCCR2B = 0;
    TCCR2A = 0;
    TCNT2 = 0;
    PCMSK0 = PCMSK1 = PCMSK2 = 0;
    DDRB = DDRC = DDRD = 0;
}

