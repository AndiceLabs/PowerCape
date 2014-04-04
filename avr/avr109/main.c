/*****************************************************************************
*
* Atmel Corporation
*
* File              : main.c
* Compiler          : IAR C 3.10C Kickstart, AVR-GCC/avr-libc(>= 1.2.5)
* Revision          : $Revision: 1.7 $
* Date              : $Date: Tuesday, June 07, 200 $
* Updated by        : $Author: raapeland $
*
* Support mail      : avr@atmel.com
*
* Target platform   : All AVRs with bootloader support
*
* AppNote           : AVR109 - Self-programming
*
* Description   : This Program allows an AVR with bootloader capabilities to
*                 Read/write its own Flash/EEprom. To enter Programming mode
*                 an input pin is checked. If this pin is pulled low, programming mode
*                 is entered. If not, normal execution is done from $0000
*                 "reset" vector in Application area.
*
* Preparations  : Use the preprocessor.xls file for obtaining a customized
*                 defines.h file and linker-file code-segment definition for
*                 the device you are compiling for.
****************************************************************************/

#include <avr/eeprom.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "defines.h"
#include "serial.h"


#define PIN_CP          ( 1 << PD6 )        // PCINT22
#define PIN_D           ( 1 << PD7 )        // PCINT23

#define EE_FLAG_LOADER      0x01

/* Uncomment the following to save code space */
//#define REMOVE_AVRPROG_SUPPORT
//#define REMOVE_FUSE_AND_LOCK_BIT_SUPPORT
//#define REMOVE_BLOCK_SUPPORT
//#define REMOVE_EEPROM_BYTE_SUPPORT
//#define REMOVE_FLASH_BYTE_SUPPORT

/*
 * GCC doesn't optimize long int arithmetics very clever.  As the
 * address only needs to be larger than 16 bits for the ATmega128 and
 * above (where flash consumptions isn't much of an issue as the
 * entire boot loader will still fit even into the smallest possible
 * boot loader section), save space by using a 16-bit variable for the
 * smaller devices.
 */
#ifdef LARGE_MEMORY
#  define ADDR_T uint32_t
#else  /* !LARGE_MEMORY */
#  define ADDR_T uint16_t
#endif /* LARGE_MEMORY */

#ifndef REMOVE_BLOCK_SUPPORT
unsigned char BlockLoad( uint16_t size, uint8_t mem, ADDR_T *address );
void BlockRead( uint16_t size, uint8_t mem, ADDR_T *address );


/* BLOCKSIZE should be chosen so that the following holds: BLOCKSIZE*n = PAGESIZE,  where n=1,2,3... */
#define BLOCKSIZE PAGESIZE

#endif /* REMOVE_BLOCK_SUPPORT */


void __jumpMain( void ) __attribute__(( naked ) ) __attribute__(( section( ".init9" ) ) );
void __jumpMain( void )
{
    asm volatile( "cli" );
    asm volatile( ".set __stack, %0" :: "i"( RAMEND ) );
    asm volatile( "clr  __zero_reg__" );         // r1 set to 0
    asm volatile( "ldi  r28, lo8(__stack)" );
    asm volatile( "ldi  r29, hi8(__stack)" );
    asm volatile( "out  0x3E, r29" );             // SPH
    asm volatile( "out  0x3D, r28" );             // SPL
    asm volatile( "rjmp main" );                   // jump to main()
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


ADDR_T address;
uint16_t temp_int;
uint8_t val;
uint8_t reason;
uint8_t flags;
uint8_t oscval;
uint8_t run_app = 1;


int main( void ) __attribute__(( naked ));
int main( void )
{
    /* Initialization */
    reason = MCUSR;
    MCUSR = 0;
    
    PORTD = 0;
    DDRD = ( PIN_CP | PIN_D );
    board_power_on();
    
    /* Disable watchdog */
    __asm__ __volatile__ ( "wdr" );
    WDTCSR |= ( 1 << WDCE ) | ( 1 << WDE );
    WDTCSR = 0x00;

    flags = eeprom_read_byte( (uint8_t*)0 );
    oscval = eeprom_read_byte( (uint8_t*)1 );

    if ( oscval != 0xFF )
    {
        OSCCAL = oscval;
    }
    
    /* Prepare application pointer */
    void ( *funcptr )( void ) = 0x0000; // Set up function pointer to RESET vector.

    /* Branch to bootloader or application code? */
    if ( pgm_read_word_near( 0 ) == 0xFFFF)
    {
        run_app = 0;
    }
    else
    {
        if ( flags & EE_FLAG_LOADER )
        {
            run_app = 0;
        }
    }

    if ( run_app == 0 )
    {
        inituart();
        
        /* Main bootloader loop */
        for ( ;; )
        {
            /* Wait for command character */
            while ( 1 )
            {
                if ( rxready() )
                {
                    val = rxchar();
                    break;
                }
            }
            
            // Set address.
            if ( val == 'A' ) // Set address...
            {
                // NOTE: Flash addresses are given in words, not bytes.
                address = ( rxchar() << 8 ) | rxchar(); // Read address high and low byte.
                txchar( '\r' ); // Send OK back.
            }
            
#ifndef REMOVE_BLOCK_SUPPORT
            // Start block load.
            else if ( val == 'B' )
            {
                temp_int = ( rxchar() << 8 ) | rxchar(); // Get block size.
                val = rxchar(); // Get memtype.
                txchar( BlockLoad( temp_int, val, &address ) ); // Block load.
            }

            // Start block read.
            else if ( val == 'g' )
            {
                temp_int = ( rxchar() << 8 ) | rxchar(); // Get block size.
                val = rxchar(); // Get memtype
                BlockRead( temp_int, val, &address ); // Block read
            }
            
            // Check block load support.
            else if ( val == 'b' )
            {
                txchar( 'Y' ); // Report block load supported.
                txchar(( BLOCKSIZE >> 8 ) & 0xFF ); // MSB first.
                txchar( BLOCKSIZE&0xFF ); // Report BLOCKSIZE (bytes).
            }            
#endif /* REMOVE_BLOCK_SUPPORT */

#ifndef REMOVE_FLASH_BYTE_SUPPORT
            // Read program memory.
            else if ( val == 'R' )
            {
                // Send high byte, then low byte of flash word.
                boot_spm_busy_wait();
                boot_rww_enable();
                txchar( pgm_read_byte_near( ( address << 1 ) + 1 ) );
                txchar( pgm_read_byte_near( ( address << 1 ) + 0 ) );
                address++; // Auto-advance to next Flash word.
            }

            // Write program memory, low byte.
            else if ( val == 'c' )
            {
                // NOTE: Always use this command before sending high byte.
                temp_int = rxchar(); // Get low byte for later _FILL_TEMP_WORD.
                txchar( '\r' ); // Send OK back.
            }

            // Write program memory, high byte.
            else if ( val == 'C' )
            {
                temp_int |= ( rxchar() << 8 ); // Get and insert high byte.
                boot_spm_busy_wait();
                boot_page_fill(( address << 1 ), temp_int ); // Convert word-address to byte-address and fill.
                address++; // Auto-advance to next Flash word.
                txchar( '\r' ); // Send OK back.
            }

            // Write page.
            else if ( val == 'm' )
            {
                if ( address >= ( APP_END >> 1 ) ) // Protect bootloader area.
                {
                    txchar( '?' );
                }
                else
                {
                    boot_spm_busy_wait();
                    boot_page_write( address << 1 ); // Convert word-address to byte-address and write.
                }

                txchar( '\r' ); // Send OK back.
            }
#endif /* REMOVE_FLASH_BYTE_SUPPORT */

#ifndef REMOVE_EEPROM_BYTE_SUPPORT
            // Write EEPROM memory.
            else if ( val == 'D' )
            {
                boot_spm_busy_wait();
                eeprom_write_byte( (uint8_t*)address, rxchar() );
                address++; // Auto-advance to next EEPROM byte.
                txchar( '\r' );// Send OK back.
            }

            // Read EEPROM memory.
            else if ( val == 'd' )
            {
                txchar( eeprom_read_byte( (uint8_t*)address ) );
                address++; // Auto-advance to next EEPROM byte.
            }
#endif /* REMOVE_EEPROM_BYTE_SUPPORT */

#ifndef REMOVE_FUSE_AND_LOCK_BIT_SUPPORT
            // Write lockbits.
            else if ( val == 'l' )
            {
                boot_spm_busy_wait();
                boot_lock_bits_set( ~rxchar() ); // Read and set lock bits.
                txchar( '\r' ); // Send OK back.
            }

#if defined(_GET_LOCK_BITS)
            // Read lock bits.
            else if ( val == 'r' )
            {
                boot_spm_busy_wait();
                txchar( boot_lock_fuse_bits_get( GET_LOCK_BITS ) );
            }

            // Read fuse bits.
            else if ( val == 'F' )
            {
                boot_spm_busy_wait();
                txchar( boot_lock_fuse_bits_get( GET_LOW_FUSE_BITS ) );
            }

            // Read high fuse bits.
            else if ( val == 'N' )
            {
                boot_spm_busy_wait();
                txchar( boot_lock_fuse_bits_get( GET_HIGH_FUSE_BITS ) );
            }

            // Read extended fuse bits.
            else if ( val == 'Q' )
            {
                boot_spm_busy_wait();
                txchar( boot_lock_fuse_bits_get( GET_EXTENDED_FUSE_BITS ) );
            }
#endif /* defined(_GET_LOCK_BITS) */
#endif /* REMOVE_FUSE_AND_LOCK_BIT_SUPPORT */

#ifndef REMOVE_AVRPROG_SUPPORT
            // Enter and leave programming mode.
            else if (( val == 'P' ) || ( val == 'L' ) )
            {
                txchar( '\r' ); // Nothing special to do, just answer OK.
            }

            // Exit bootloader.
            else if ( val == 'E' )
            {
                boot_spm_busy_wait();
                boot_rww_enable();
                txchar( '\r' );
                eeprom_update_byte( (uint8_t*)0, flags & ~EE_FLAG_LOADER );
                funcptr(); // Jump to Reset vector 0x0000 in Application Section.
            }

            // Get programmer type.
            else if ( val == 'p' )
            {
                txchar( 'S' ); // Answer 'SERIAL'.
            }

            // Return supported device codes.
            else if ( val == 't' )
            {
#if PARTCODE+0 > 0
                txchar( PARTCODE ); // Supports only this device, of course.
#endif /* PARTCODE */
                txchar( 0 ); // Send list terminator.
            }

            // Set LED, clear LED and set device type.
            else if (( val == 'x' ) || ( val == 'y' ) || ( val == 'T' ) )
            {
                rxchar(); // Ignore the command and it's parameter.
                txchar( '\r' ); // Send OK back.
            }

#endif /* REMOVE_AVRPROG_SUPPORT */

            // Chip erase.
            else if ( val == 'e' )
            {
                for ( address = 0; address < APP_END;address += PAGESIZE )
                {
                    // NOTE: Here we use address as a byte-address, not word-address, for convenience.
                    boot_page_erase( address );
                    boot_spm_busy_wait();
                }

                txchar( '\r' ); // Send OK back.
            }

            // Return programmer identifier.
            else if ( val == 'S' )
            {
                txchar( 'A' ); // Return 'AVRBOOT'.
                txchar( 'V' ); // Software identifier (aka programmer signature) is always 7 characters.
                txchar( 'R' );
                txchar( 'B' );
                txchar( 'O' );
                txchar( 'O' );
                txchar( 'T' );
            }

            // Return software version.
            else if ( val == 'V' )
            {
                txchar( '1' );
                txchar( '5' );
            }

            // Return signature bytes.
            else if ( val == 's' )
            {
                txchar( SIGNATURE_BYTE_3 );
                txchar( SIGNATURE_BYTE_2 );
                txchar( SIGNATURE_BYTE_1 );
            }

            // Check autoincrement status.
            else if ( val == 'a' )
            {
                txchar( 'Y' ); // Yes, we do autoincrement.
            }
            
            // The last command to accept is ESC (synchronization).
            else if ( val != 0x1b )             // If not ESC, then it is unrecognized...
            {
                txchar( '?' );
            }
        } // end: for(;;)
    }
    else
    {
        boot_spm_busy_wait();
        boot_rww_enable();
        funcptr(); // Jump to Reset vector 0x0000 in Application Section.
    }
} // end: main


#ifndef REMOVE_BLOCK_SUPPORT
unsigned char BlockLoad( unsigned int size, unsigned char mem, ADDR_T *address )
{
    unsigned char buffer[BLOCKSIZE];
    unsigned int data;
    ADDR_T tempaddress;

    // EEPROM memory type.
    if ( mem == 'E' )
    {
        /* Fill buffer first, as EEPROM is too slow to copy with UART speed */
        for ( tempaddress = 0;tempaddress < size;tempaddress++ )
            buffer[tempaddress] = rxchar();

        /* Then program the EEPROM */
        boot_spm_busy_wait();
        
        for ( tempaddress = 0; tempaddress < size; tempaddress++ )
        {
            eeprom_write_byte( (uint8_t*)(*address), buffer[ tempaddress ] );
            ( *address )++; // Select next EEPROM byte
        }

        return '\r'; // Report programming OK
    }

    // Flash memory type.
    else if ( mem == 'F' )
    {
        // NOTE: For flash programming, 'address' is given in words.
        ( *address ) <<= 1; // Convert address to bytes temporarily.
        tempaddress = ( *address );  // Store address in page.

        //eeprom_busy_wait();
        //boot_page_erase( tempaddress );
        boot_spm_busy_wait();
        
        do
        {
            data = rxchar();
            data |= ( rxchar() << 8 );
            boot_page_fill( *address, data );
            ( *address ) += 2; // Select next word in memory.
            size -= 2; // Reduce number of bytes to write by two.
        }
        while ( size ); // Loop until all bytes written.

        boot_page_write( tempaddress );
        boot_spm_busy_wait();

        boot_rww_enable();

        ( *address ) >>= 1; // Convert address back to Flash words again.

        return '\r'; // Report programming OK
    }

    // Invalid memory type?
    else
    {
        return '?';
    }
}


void BlockRead( unsigned int size, unsigned char mem, ADDR_T *address )
{
    // EEPROM memory type.
    if ( mem == 'E' ) // Read EEPROM
    {
        do
        {
            txchar( eeprom_read_byte( (uint8_t*)(*address) ) );
            ( *address )++; // Select next EEPROM byte
            size--; // Decrease number of bytes to read
        }
        while ( size ); // Repeat until all block has been read
    }

    // Flash memory type.
    else if ( mem == 'F' )
    {
        ( *address ) <<= 1; // Convert address to bytes temporarily.

        do
        {
            txchar( pgm_read_byte_near( *address ) );
            txchar( pgm_read_byte_near( ( *address ) + 1 ) );
            ( *address ) += 2; // Select next word in memory.
            size -= 2; // Subtract two bytes from number of bytes to read
        }
        while ( size ); // Repeat until all block has been read

        ( *address ) >>= 1; // Convert address back to Flash words again.
    }
}

#endif /* REMOVE_BLOCK_SUPPORT */


/* end of file */
