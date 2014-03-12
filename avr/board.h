#ifndef __BOARD_H__
#define __BOARD_H__

// Port B
#define PIN_LED1        ( 1 << PB0 )        // PCINT0
#define PIN_LED0        ( 1 << PB1 )        // PCINT1 
#define PIN_OPTO        ( 1 << PB2 )        // PCINT2
#define PIN_PB3         ( 1 << PB3 )        // PCINT3
#define PIN_PB4         ( 1 << PB4 )        // PCINT4
#define PIN_PB5         ( 1 << PB5 )        // PCINT5
#define PIN_XTAL1       ( 1 << PB6 )        // XTAL1
#define PIN_XTAL2       ( 1 << PB7 )        // XTAL2

// Port C
#define PIN_CE          ( 1 << PC0 )        // NC on P1 board
#define PIN_PC1         ( 1 << PC1 )        // NC on P0 board
#define PIN_PGOOD       ( 1 << PC2 )        // PCINT10
#define PIN_ADC3        ( 1 << PC3 )        // NC
#define PIN_SDA         ( 1 << PC4 )        // I2C
#define PIN_SCL         ( 1 << PC5 )        // I2C

// Port D
#define PIN_RXD         ( 1 << PD0 )        // PCINT16
#define PIN_TXD         ( 1 << PD1 )        // PCINT17
#define PIN_ENABLE      ( 1 << PD2 )        // PCINT18
#define PIN_DETECT      ( 1 << PD3 )        // PCINT19
#define PIN_PD4         ( 1 << PD4 )        // PCINT20
#define PIN_BUTTON      ( 1 << PD5 )        // PCINT21
#define PIN_CP          ( 1 << PD6 )        // PCINT22
#define PIN_D           ( 1 << PD7 )        // PCINT23


// This is for using WDT as a timer, not system reset
#define wdt_timer_enable(value)   \
__asm__ __volatile__ (  \
    "in __tmp_reg__,__SREG__" "\n\t"    \
    "cli" "\n\t"    \
    "wdr" "\n\t"    \
    "sts %0,%1" "\n\t"  \
    "sts %0,%2" "\n\t" \
    "out __SREG__,__tmp_reg__" "\n\t"   \
    : /* no outputs */  \
    : "M" (_SFR_MEM_ADDR(_WD_CONTROL_REG)), \
    "r" ( _BV(_WD_CHANGE_BIT) | _BV(WDIE) | _BV(WDE) | (value & 0x07) ), \
    "r" ( (uint8_t) ( _BV(WDIE) | (value & 0x07)) ) \
    : "r0"  \
)


void timer0_init( void );
void timer1_init( void );

uint8_t board_begin_countdown( void );

void board_power_on( void );
void board_power_off( void );

void board_led_on( uint8_t led );
void board_led_off( uint8_t led );

void board_ce( uint8_t enable );

uint8_t board_3v3( void );
uint8_t board_pgood( void );

void board_enable_interrupt( uint8_t mask );
void board_disable_interrupt( uint8_t mask );

void board_gpio_config( void );
void board_init( void );


#endif  // __BOARD_H__

