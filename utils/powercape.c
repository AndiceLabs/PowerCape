#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <endian.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include "../avr/registers.h"

#define AVR_ADDRESS         0x21
#define INA_ADDRESS         0x40


typedef enum {
    OP_NONE,
    OP_BOOT,
    OP_QUERY,
    OP_READ_RTC,
    OP_SET_SYSTIME,
    OP_WRITE_RTC
} op_type;

op_type operation = OP_NONE;

int i2c_bus = 1;
int handle;


void msleep( int msecs )
{
    usleep( msecs * 1000 );
}


int i2c_read( void *buf, int len )
{
    int rc = 0;

    if ( read( handle, buf, len ) != len )
    {
        printf( "I2C read failed: %s\n", strerror( errno ) );
        rc = -1;
    }

    return rc;
}


int i2c_write( void *buf, int len )
{
    int rc = 0;
    
    if ( write( handle, buf, len ) != len ) 
    {
        printf( "I2C write failed: %s\n", strerror( errno ) );
        rc = -1;
    }
    
    return rc;
}


int register_read( unsigned char reg, unsigned char *data )
{
    int rc = -1;
    unsigned char bite[ 4 ];
    
    bite[ 0 ] = reg;
    if ( i2c_write( bite, 1 ) == 0 )
    {
        if ( i2c_read( bite, 1 ) == 0 )
        {
            *data = bite[ 0 ];
            rc = 0;
        }
    }
    
    return rc;
}


int register32_read( unsigned char reg, unsigned int *data )
{
    int rc = -1;
    unsigned char bite[ 4 ];
    
    bite[ 0 ] = reg;
    if ( i2c_write( bite, 1 ) == 0 )
    {
        if ( i2c_read( data, 4 ) == 0 )
        {
            rc = 0;
        }
    }
    
    return rc;
}


int register_write( unsigned char reg, unsigned char data )
{
    int rc = -1;
    unsigned char bite[ 4 ];
    
    bite[ 0 ] = reg;
    bite[ 1 ] = data;

    if ( i2c_write( bite, 2 ) == 0 )
    {
        rc = 0;
    }
    
    return rc;
}


int register32_write( unsigned char reg, unsigned int data )
{
    int rc = -1;
    unsigned char bite[ 6 ];
    
    bite[ 0 ] = reg;
    bite[ 1 ] = data & 0xFF;
    bite[ 2 ] = ( data >> 8 ) & 0xFF;
    bite[ 3 ] = ( data >> 16 ) & 0xFF;
    bite[ 4 ] = ( data >> 24 ) & 0xFF;

    if ( i2c_write( bite, 5 ) == 0 )
    {
        rc = 0;
    }
    
    return rc;
}


int cape_enter_bootloader( void )
{
    unsigned char b;
    int rc = 2;
    
    if ( register_write( REG_CONTROL, CONTROL_BOOTLOAD ) == 0 )
    {
        if ( register_read( REG_CONTROL, &b ) == 0 )
        {
            fprintf( stderr, "Unable to switch to cape bootloader\n" );
            rc = 3;
        }
        else
        {
            rc = 0;
        }
    }
    
    return rc;
}


int cape_read_rtc( time_t *iptr )
{
    int rc = 1;
    unsigned int seconds;
    
    if ( register32_read( REG_SECONDS_0, &seconds ) == 0 )
    {
        //printf( "Cape RTC seconds %08X (%d)\n", seconds, seconds );
        printf( "%s\n", ctime( (time_t*)&seconds ) );
        
        if ( iptr != NULL )
        {
            *iptr = seconds;
        }
        rc = 0;
    }
    
    return rc;
}


int cape_write_rtc( void )
{
    int rc = 1;
    unsigned int seconds = time( NULL );
    
    //printf( "System seconds %08X (%d)\n", seconds, seconds );
    printf( "%s\n", ctime( (time_t*)&seconds ) );

    if ( register32_write( REG_SECONDS_0, seconds ) == 0 )
    {
        rc = 0;
    }
    
    return rc;
}


void show_usage( char *progname )
{
    fprintf( stderr, "Usage: %s <mode> \n", progname );
    fprintf( stderr, "   Mode (required):\n" );
    fprintf( stderr, "      -h --help           Show usage.\n" );
    fprintf( stderr, "      -b --boot           Enter bootloader.\n" );
    fprintf( stderr, "      -q --query <reason> Query if reason was cause of power-on.\n" );
    fprintf( stderr, "                          <reason> can be TMO, PWR, BUT, or EXT.\n" );
    fprintf( stderr, "      -r --read           Read cape RTC value.\n" );
    fprintf( stderr, "      -s --set            Set system time from cape RTC.\n" );
    fprintf( stderr, "      -w --write          Write cape RTC from system time.\n" );
    exit( 1 );
}


void parse( int argc, char *argv[] )
{
    while( 1 )
    {
        static const struct option lopts[] =
        {
            { "help",       0, 0, 'h' },
            { "boot",       0, 0, 'b' },
            { "query",      0, 0, 'q' },
            { "read",       0, 0, 'r' },
            { "set",        0, 0, 's' },
            { "write",      0, 0, 'w' },
            { NULL,         0, 0, 0 },
        };
        int c;

        c = getopt_long( argc, argv, "hbq:rsw", lopts, NULL );

        if( c == -1 )
            break;

        switch( c )
        {
            case 'b':
            {
                operation = OP_BOOT;
                break;
            }

            case 'q':
            {
                operation = OP_QUERY;
                //interval = atoi( optarg );
                break;
            }
            
            case 'r':
            {
                operation = OP_READ_RTC;
                break;
            }
            
            case 's':
            {
                operation = OP_SET_SYSTIME;
                break;
            }
            
            case 'w':
            {
                operation = OP_WRITE_RTC;
                break;
            }
            
            case 'h':
            {
                operation = OP_NONE;
                show_usage( argv[ 0 ] );
                break;
            }
        }
    }
}


int main( int argc, char *argv[] )
{
    int rc = 0;
    char filename[ 20 ];

    snprintf( filename, 19, "/dev/i2c-%d", i2c_bus );
    handle = open( filename, O_RDWR );
    if ( handle < 0 ) 
    {
        fprintf( stderr, "Error opening device: %s\n", strerror( errno ) );
        exit( 1 );
    }

    if ( ioctl( handle, I2C_SLAVE, AVR_ADDRESS ) < 0 ) 
    {
        fprintf( stderr, "IOCTL Error: %s\n", strerror( errno ) );
        exit( 1 );
    }

    parse( argc, argv );

    switch ( operation )
    {
        case OP_QUERY:
        {
            break;
        }

        case OP_BOOT:
        {
            rc = cape_enter_bootloader();
            break;
        }

        case OP_READ_RTC:
        {
            rc = cape_read_rtc( NULL );
            break;
        }

        case OP_SET_SYSTIME:
        {
            struct timeval t;
            
            rc = cape_read_rtc( &t.tv_sec );
            if ( rc == 0 )
            {
                t.tv_usec = 0;
                rc = settimeofday( &t, NULL);
                if ( rc != 0 )
                {
                    fprintf( stderr, "Error: %s\n", strerror( errno ) );
                }
            }
            break;
        }
        
        case OP_WRITE_RTC:
        {
            rc = cape_write_rtc();
            break;
        }
        
        default:
        case OP_NONE:
        {
            break;
        }
    }

    close( handle );
    return rc;
}

