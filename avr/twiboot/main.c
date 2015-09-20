/***************************************************************************
 *   Copyright (C) 08/2010 by Olaf Rempel                                  *
 *   razzor@kopf-tisch.de                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License,               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define FALSE   0
#define TRUE    1

/*
 * atmega88p:
 * Fuse E: 0x00 (1024 words bootloader)
 * Fuse H: 0xd7 (BOD disabled)
 * Fuse L: 0xe2 (8Mhz internal RC-Osc.)
 *
 * atmega168p:
 * Fuse E: 0x00 (1024 words bootloader)
 * Fuse H: 0xd7 (BOD disabled)
 * Fuse L: 0xe2 (8Mhz internal RC-Osc.)
 * 
 * atmega328p:
 * Fuse E: 0x07 (BOD disabled)
 * Fuse H: 0xd2 (1024 words bootloader)
 * Fuse L: 0xe2 (8Mhz internal RC-Osc.)
 */

#if defined (__AVR_ATmega8__)
#define VERSION_STRING		    "TWIBOOT v2.1d"
#define SIGNATURE_BYTES		    0x1E, 0x93, 0x07

#elif defined (__AVR_ATmega88P__)
#define VERSION_STRING		    "TWIBOOT v2.1d"
#define SIGNATURE_BYTES		    0x1E, 0x93, 0x0F

#elif defined (__AVR_ATmega168P__)
#define VERSION_STRING		    "TWIBOOT v2.1d"
#define SIGNATURE_BYTES		    0x1E, 0x94, 0x0B

#elif defined (__AVR_ATmega328P__)
#define VERSION_STRING          "TWIBOOT v2.1d"
#define SIGNATURE_BYTES         0x1E, 0x95, 0x0F

#else
#error MCU not supported
#endif

#define EEPROM_SUPPORT		    1

/* 25ms @8MHz */
#define TIMER_RELOAD		    ( 0xFF - 195 )

/* 40 * 25ms */
#define TIMEOUT			        40

#define LED_INIT()		        DDRB = ( ( 1 << PB0 ) | ( 1 << PB1 ) )
#define LED_RT_ON()		        PORTB |= ( 1 << PB0 )
#define LED_RT_OFF()		    PORTB &= ~( 1 << PB0 )
#define LED_GN_ON()		        PORTB |= ( 1 << PB1 )
#define LED_GN_OFF()		    PORTB &= ~( 1 << PB1 )
#define LED_GN_TOGGLE()		    PORTB ^= ( 1 << PB1 )
#define LED_OFF()		        PORTB = 0x00

#define EEPROM_FLAGS            ( (uint8_t*)0 )
#define EE_FLAG_LOADER          0x01
#define PIN_CP                  ( 1 << PD6 )        // PCINT22
#define PIN_D                   ( 1 << PD7 )        // PCINT23
#define BOARD_INIT()            DDRD = ( ( 1 << PD6 ) | ( 1 << PD7 ) )

#define TWI_ADDRESS		        ( 0x20 << 1 )

/* SLA+R */
#define CMD_WAIT		        0x00
#define CMD_READ_VERSION	    0x01
#define CMD_READ_MEMORY		    0x02
/* internal mappings */
#define CMD_READ_CHIPINFO	    ( 0x10 | CMD_READ_MEMORY )
#define CMD_READ_FLASH		    ( 0x20 | CMD_READ_MEMORY )
#define CMD_READ_EEPROM		    ( 0x30 | CMD_READ_MEMORY )
#define CMD_READ_PARAMETERS	    ( 0x40 | CMD_READ_MEMORY )	/* only in APP */

/* SLA+W */
#define CMD_SWITCH_APPLICATION	CMD_READ_VERSION
#define CMD_WRITE_MEMORY	    CMD_READ_MEMORY
/* internal mappings */
#define CMD_BOOT_BOOTLOADER	    ( 0x10 | CMD_SWITCH_APPLICATION)	/* only in APP */
#define CMD_BOOT_APPLICATION	( 0x20 | CMD_SWITCH_APPLICATION)
#define CMD_WRITE_CHIPINFO	    ( 0x10 | CMD_WRITE_MEMORY)	/* invalid */
#define CMD_WRITE_FLASH		    ( 0x20 | CMD_WRITE_MEMORY)
#define CMD_WRITE_EEPROM	    ( 0x30 | CMD_WRITE_MEMORY)
#define CMD_WRITE_PARAMETERS	( 0x40 | CMD_WRITE_MEMORY)	/* only in APP */

/* CMD_SWITCH_APPLICATION parameter */
#define BOOTTYPE_BOOTLOADER	    0x00				/* only in APP */
#define BOOTTYPE_APPLICATION	0x80

/* CMD_{READ|WRITE}_* parameter */
#define MEMTYPE_CHIPINFO	    0x00
#define MEMTYPE_FLASH		    0x01
#define MEMTYPE_EEPROM		    0x02
#define MEMTYPE_PARAMETERS	    0x03				/* only in APP */

/*
 * LED_GN blinks with 20Hz (while bootloader is running)
 * LED_RT blinks on TWI activity
 *
 * bootloader twi-protocol:
 * - abort boot timeout:
 *   SLA+W, 0x00, STO
 *
 * - show bootloader version
 *   SLA+W, 0x01, SLA+R, {16 bytes}, STO
 *
 * - start application
 *   SLA+W, 0x01, 0x80, STO
 *
 * - read chip info: 3byte signature, 1byte page size, 2byte flash size, 2byte eeprom size
 *   SLA+W, 0x02, 0x00, 0x00, 0x00, SLA+R, {4 bytes}, STO
 *
 * - read one (or more) flash bytes
 *   SLA+W, 0x02, 0x01, addrh, addrl, SLA+R, {* bytes}, STO
 *
 * - read one (or more) eeprom bytes
 *   SLA+W, 0x02, 0x02, addrh, addrl, SLA+R, {* bytes}, STO
 *
 * - write one flash page (64bytes on mega8)
 *   SLA+W, 0x02, 0x01, addrh, addrl, {64 bytes}, STO
 *
 * - write one (or more) eeprom bytes
 *   SLA+W, 0x02, 0x02, addrh, addrl, {* bytes}, STO
 */

const static uint8_t info[16] = VERSION_STRING;
const static uint8_t chipinfo[8] =
{
    SIGNATURE_BYTES,

    SPM_PAGESIZE,

    ( APP_END >> 8 ) & 0xFF,
    APP_END & 0xFF,
#if (EEPROM_SUPPORT)
    ( ( E2END + 1 ) >> 8 & 0xFF ),
    ( E2END + 1 ) & 0xFF
#else
    0x00, 0x00
#endif
};

/* wait 40 * 25ms = 1s */
//static uint8_t boot_timeout = TIMEOUT;
volatile static uint8_t cmd = CMD_WAIT;

/* flash buffer */
static uint8_t buf[ SPM_PAGESIZE ];
static uint16_t addr;

uint8_t no_activity = 1;
uint8_t run_app = 1;
uint8_t mcusr __attribute__ ((section (".noinit")));


void timer2_init( uint8_t enable )
{
    PRR &= ~( 1 << PRTIM2 );    // is this necessary with async mode?
    ASSR = ( 1 << AS2 );        // external crystal
    TCCR2B = 0;
    TCCR2A = 0;
    TCNT2 = 0;
    TIFR2 = TIFR2;              // clear any flags
    
    if ( enable != 0 )
    {
        TCCR2B = ( 1 << CS22 ) | ( 1 << CS21 ) | ( 1 << CS20 );    // clk/1024 (8s)
    }
}


void board_power( void )
{
    PORTD |= PIN_D;
    asm volatile( "nop\n\t" );
    PORTD |= PIN_CP;
    asm volatile( "nop\n\t" );
    PORTD &= ~PIN_CP;
    PORTD &= ~PIN_D;
}


static void write_flash_page( void )
{
    uint16_t pagestart = addr;
    uint8_t size = SPM_PAGESIZE;
    uint8_t *p = buf;

    if( pagestart >= APP_END )
        return;

    boot_page_erase( pagestart );
    boot_spm_busy_wait();

    do
    {
        uint16_t data = *p++;
        data |= *p++ << 8;
        boot_page_fill( addr, data );

        addr += 2;
        size -= 2;
    }
    while( size );

    boot_page_write( pagestart );
    boot_spm_busy_wait();
    boot_rww_enable();
}

#if (EEPROM_SUPPORT)
static uint8_t read_eeprom_byte( void )
{
    EEARL = addr;
    EEARH = ( addr >> 8 );
    EECR |= ( 1 << EERE );
    addr++;
    return EEDR;
}


static void write_eeprom_byte( uint8_t val )
{
    EEARL = addr;
    EEARH = ( addr >> 8 );
    EEDR = val;
    addr++;
#if defined (__AVR_ATmega8__)
    EECR |= ( 1 << EEMWE );
    EECR |= ( 1 << EEWE );
#elif defined (__AVR_ATmega88P__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)
    EECR |= ( 1 << EEMPE );
    EECR |= ( 1 << EEPE );
#endif
    eeprom_busy_wait();
}
#endif /* EEPROM_SUPPORT */


ISR( TWI_vect )
{
    static uint8_t bcnt;
    uint8_t data;
    uint8_t ack = ( 1 << TWEA );
	uint8_t flash_it = 0;

    switch( TWSR & 0xF8 )
    {
            /* SLA+W received, ACK returned -> receive data and ACK */
        case 0x60:
            if ( no_activity )
            {
                timer2_init( FALSE );
                no_activity = 0;
            }
            bcnt = 0;
            LED_RT_ON();
            TWCR |= ( 1 << TWINT ) | ( 1 << TWEA );
            break;

            /* prev. SLA+W, data received, ACK returned -> receive data and ACK */
        case 0x80:
            data = TWDR;

            switch( bcnt )
            {
                case 0:

                    switch( data )
                    {
                        case CMD_SWITCH_APPLICATION:
                        case CMD_WRITE_MEMORY:
                            bcnt++;
                            /* no break */

                        case CMD_WAIT:
                            /* abort countdown */
                            //boot_timeout = 0;
                            break;

                        default:
                            /* boot app now */
                            cmd = CMD_BOOT_APPLICATION;
                            ack = ( 0 << TWEA );
                            break;
                    }

                    cmd = data;
                    break;

                case 1:

                    switch( cmd )
                    {
                        case CMD_SWITCH_APPLICATION:

                            if( data == BOOTTYPE_APPLICATION )
                            {
                                cmd = CMD_BOOT_APPLICATION;
                            }

                            ack = ( 0 << TWEA );
                            break;

                        case CMD_WRITE_MEMORY:
                            bcnt++;

                            if( data == MEMTYPE_CHIPINFO )
                            {
                                cmd = CMD_WRITE_CHIPINFO;

                            }
                            else if( data == MEMTYPE_FLASH )
                            {
                                cmd = CMD_WRITE_FLASH;
#if (EEPROM_SUPPORT)
                            }
                            else if( data == MEMTYPE_EEPROM )
                            {
                                cmd = CMD_WRITE_EEPROM;
#endif
                            }
                            else
                            {
                                ack = ( 0 << TWEA );
                            }

                            break;

                        default:
                            ack = ( 0 << TWEA );
                            break;
                    }

                    break;

                case 2:
                case 3:
                    addr <<= 8;
                    addr |= data;
                    bcnt++;
                    break;

                default:

                    switch( cmd )
                    {
                        case CMD_WRITE_FLASH:
                            buf[bcnt - 4] = data;

                            if( bcnt < sizeof( buf ) + 3 )
                            {
                                bcnt++;
                            }
                            else
                            {
                                flash_it = 1;
                                //write_flash_page();
                                ack = ( 0 << TWEA );
                            }

                            break;
#if (EEPROM_SUPPORT)
                        case CMD_WRITE_EEPROM:
                            flash_it = 2;
                            //write_eeprom_byte( data );
                            bcnt++;
                            break;
#endif
                        default:
                            ack = ( 0 << TWEA );
                            break;
                    }

                    break;
            }

            if( ack == 0x00 )
                bcnt = 0;

            TWCR |= ( 1 << TWINT ) | ack;

            if (flash_it == 1)
                write_flash_page();
#if (EEPROM_SUPPORT)
            if (flash_it == 2)
                write_eeprom_byte(data);
#endif
            break;

            /* SLA+R received, ACK returned -> send data */
        case 0xA8:
            if ( no_activity )
            {
                timer2_init( FALSE );
                no_activity = 0;
            }
            bcnt = 0;
            LED_RT_ON();
            // intentional fall-thru
            
            /* prev. SLA+R, data sent, ACK returned -> send data */
        case 0xB8:

            switch( cmd )
            {
                case CMD_READ_VERSION:
                    data = info[bcnt++];
                    bcnt %= sizeof( info );
                    break;

                case CMD_READ_CHIPINFO:
                    data = chipinfo[bcnt++];
                    bcnt %= sizeof( chipinfo );
                    break;

                case CMD_READ_FLASH:
                    data = pgm_read_byte_near( addr++ );
                    break;
#if (EEPROM_SUPPORT)
                case CMD_READ_EEPROM:
                    data = read_eeprom_byte();
                    break;
#endif
                default:
                    data = 0xFF;
                    break;
            }

            TWDR = data;
            TWCR |= ( 1 << TWINT ) | ( 1 << TWEA );
            break;

            /* STOP or repeated START */
        case 0xA0:
            /* data sent, NACK returned */
        case 0xC0:
            LED_RT_OFF();
            TWCR |= ( 1 << TWINT ) | ( 1 << TWEA );
            break;

            /* illegal state -> reset hardware */
        case 0xF8:
            TWCR |= ( 1 << TWINT ) | ( 1 << TWSTO ) | ( 1 << TWEA );
            break;
    }
}


ISR( TIMER0_OVF_vect )
{
    /* restart timer */
    TCNT0 = TIMER_RELOAD;

    /* blink LED while running */
    LED_GN_TOGGLE();
}


static void ( *jump_to_app )( void ) __attribute__( ( noreturn ) ) = 0x0000;

/*
 * For newer devices (mega88) the watchdog timer remains active even after a
 * system reset. So disable it as soon as possible.
 * automagically called on startup
 */
#if defined (__AVR_ATmega88P__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)
void disable_wdt_timer( void ) __attribute__( ( naked, section( ".init3" ) ) );
void disable_wdt_timer( void )
{
    mcusr = MCUSR;
    MCUSR = 0;
    WDTCSR = ( 1 << WDCE ) | ( 1 << WDE );
    WDTCSR = ( 0 << WDE );
}
#endif


int main( void ) __attribute__( ( noreturn ) );
int main( void )
{
    /* Bootloader or application code? */
    if ( pgm_read_word_near( 0 ) == 0xFFFF)
    {
        timer2_init( FALSE );
        run_app = 0;
    }
    else
    {
        if ( mcusr & ( 1 << WDRF ) )
        {
            timer2_init( TRUE );
            run_app = 0;
        }
    }
    
    if ( run_app == 0 )
    {
        BOARD_INIT();
        LED_INIT();
        LED_GN_ON();
        board_power();

        /* move interrupt-vectors to bootloader */
        /* timer0: running with F_CPU/1024, OVF interrupt */
#if defined (__AVR_ATmega8__)
        GICR = ( 1 << IVCE );
        GICR = ( 1 << IVSEL );

        TCCR0 = ( 1 << CS02 ) | ( 1 << CS00 );
        TIMSK = ( 1 << TOIE0 );
#elif defined (__AVR_ATmega88P__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)
        MCUCR = ( 1 << IVCE );
        MCUCR = ( 1 << IVSEL );

        TCCR0B = ( 1 << CS02 ) | ( 1 << CS00 );
        TIMSK0 = ( 1 << TOIE0 );
#endif

        /* TWI init: set address, auto ACKs with interrupts */
        TWAR = TWI_ADDRESS;
        TWCR = ( 1 << TWEA ) | ( 1 << TWEN ) | ( 1 << TWIE );

        sei();

        while( ( cmd != CMD_BOOT_APPLICATION )&& ( TIFR2 == 0 ) );

        cli();
        
        timer2_init( FALSE );

        /* Disable TWI but keep address! */
        TWCR = 0x00;

        /* disable timer0 */
        /* move interrupt vectors back to application */
#if defined (__AVR_ATmega8__)
        TCCR0 = 0x00;
        TIMSK = 0x00;

        GICR = ( 1 << IVCE );
        GICR = ( 0 << IVSEL );
#elif defined (__AVR_ATmega88P__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)
        TIMSK0 = 0x00;
        TCCR0B = 0x00;

        MCUCR = ( 1 << IVCE );
        MCUCR = ( 0 << IVSEL );
#endif

        LED_OFF();
    }
    
    wdt_enable( WDTO_1S );
    
    jump_to_app();
}
