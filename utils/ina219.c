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

#define CONFIG_REG          0
#define SHUNT_REG           1
#define BUS_REG             2
#define POWER_REG           3
#define CURRENT_REG         4
#define CALIBRATION_REG     5

#define AVR_ADDRESS         0x21
#define INA_ADDRESS         0x40

#define DEVICE_INA219       0
#define DEVICE_INA3221      1

typedef enum {
    OP_DUMP,
    OP_VOLTAGE,
    OP_CURRENT,
    OP_MONITOR,
    OP_NONE
} op_type;

op_type operation = OP_DUMP;

int device = DEVICE_INA219;
int interval = 60;
int i2c_bus = 1;
int i2c_address = INA_ADDRESS;
int handle;
int whole_numbers = 0;


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


int register_read( unsigned char reg, unsigned short *data )
{
    int rc = -1;
    unsigned char bite[ 4 ];
    
    bite[ 0 ] = reg;
    if ( i2c_write( bite, 1 ) == 0 )
    {
        if ( i2c_read( bite, 2 ) == 0 )
        {
            *data = ( bite[ 0 ] << 8 ) | bite[ 1 ];
            rc = 0;
        }
    }
    
    return rc;
}


int register_write( unsigned char reg, unsigned short data )
{
    int rc = -1;
    unsigned char bite[ 4 ];
    
    bite[ 0 ] = reg;
    bite[ 1 ] = ( data >> 8 ) & 0xFF;
    bite[ 2 ] = ( data & 0xFF );

    if ( i2c_write( bite, 3 ) == 0 )
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
    fprintf( stderr, "      -3 --ina3221        Triple channel INA3221.\n" );
    fprintf( stderr, "      -i --interval       Set interval for monitor mode.\n" );
    fprintf( stderr, "      -w --whole          Show whole numbers only. Useful for scripts.\n" );
    fprintf( stderr, "      -v --voltage        Show channel voltage in mV.\n" );
    fprintf( stderr, "      -c --current        Show channel current in mA.\n" );
    fprintf( stderr, "      -a --address <addr> Override I2C address of INA219 from default of 0x%02X.\n", i2c_address );
    fprintf( stderr, "      -b --bus <i2c bus>  Override I2C bus from default of %d.\n", i2c_bus );
    exit( 1 );
}


void parse( int argc, char *argv[] )
{
    while( 1 )
    {
        static const struct option lopts[] =
        {
            { "ina3221",    0, 0, '3' },
            { "address",    0, 0, 'a' },
            { "bus",        0, 0, 'b' },
            { "current",    0, 0, 'c' },
            { "help",       0, 0, 'h' },
            { "interval",   0, 0, 'i' },
            { "voltage",    0, 0, 'v' },
            { "whole",      0, 0, 'w' },
            { NULL,         0, 0, 0 },
        };
        int c;

        c = getopt_long( argc, argv, "3a:b:chi:vw", lopts, NULL );

        if( c == -1 )
            break;

        switch( c )
        {
            case '3':
            {
                device = DEVICE_INA3221;
                break;
            }
            case 'a':
            {
                errno = 0;
                i2c_address = (int)strtol( optarg, NULL, 0 );
                if ( errno != 0 )
                {
                    fprintf( stderr, "Unknown address parameter %s.\n", optarg );
                    exit( 1 );
                }
                break;
            }

            case 'b':
            {
                errno = 0;
                i2c_bus = (int)strtol( optarg, NULL, 0 );
                if ( errno != 0 )
                {
                    fprintf( stderr, "Unknown bus parameter %s.\n", optarg );
                    exit( 1 );
                }
                break;
            }

            case 'c':
            {
                operation = OP_CURRENT;
                break;
            }

            default:
            case 'h':
            {
                operation = OP_NONE;
                show_usage( argv[ 0 ] );
                break;
            }

            case 'i':
            {
                operation = OP_MONITOR;
                interval = atoi( optarg );
                if ( ( interval == 0 ) && ( errno != 0 ) )
                {
                    fprintf( stderr, "Invalid interval value\n" );
                    exit( 1 );
                }
                break;
            }

            case 'v':
            {
                operation = OP_VOLTAGE;
                break;
            }

            case 'w':
            {
                whole_numbers = 1;
                break;
            }
        }
    }
}


int get_voltage( int channel, float *mv )
{
    short bus;

    if ( register_read( BUS_REG + ( channel << 1 ), (unsigned short*)&bus ) != 0 )
    {
        return -1;
    }

    if ( device == DEVICE_INA219 )
    {
        *mv = ( float )( ( bus & 0xFFF8 ) >> 1 );
    } 
    else 
    {
        *mv = ( float )( bus & 0xFFF8 );
    }
    return 0;
}


int get_current( int channel, float *ma )
{
    short shunt;

    if ( register_read( SHUNT_REG + ( channel << 1 ), &shunt ) != 0 )
    {
        return -1;
    }

    if ( device == DEVICE_INA219 )
    {
        *ma = (float)shunt / 10;
    }
    else
    {
        *ma = (float)shunt / 4;
    }
    return 0;
}


void show_current( void  )
{
    int channels = 1;
    int i;
    float ma;

    if ( device == DEVICE_INA3221 )
        channels = 3;

    for ( i = 0; i < channels; i++ )
    {
        if ( get_current( i, &ma ) )
        {
            fprintf( stderr, "Error reading current\n" );
            return;
        }
        
        if ( whole_numbers )
        {
            printf( "%4.0f\n", ma );
        }
        else
        {
            printf( "%04.1f\n", ma );
        }
    }
}


void show_voltage( void )
{
    int channels = 1;
    int i;
    float mv;

    if ( device == DEVICE_INA3221 )
        channels = 3;

    for ( i = 0; i < channels; i++ )
    {
        if ( get_voltage( i, &mv ) )
        {
            fprintf( stderr, "Error reading voltage\n" );
            return;
        }
        printf( "%4.0f\n", mv );
    }
}


void show_voltage_current( void )
{
    int channels = 1;
    int i;
    float mv, ma;

    if ( device == DEVICE_INA3221 )
        channels = 3;
    
    for ( i = 0; i < channels; i++ )
    {
        if ( get_current( i, &ma ) || get_voltage( i, &mv ) )
        {
            fprintf( stderr, "Error reading voltage/current\n" );
            return;
        }

        if ( whole_numbers )
        {
            printf( "%4.0fmV  %4.0fmA\n", mv, ma );
        }
        else
        {
            printf( "%4.0fmV  %4.1fmA\n", mv, ma );
        }
    }
}


void monitor( void )
{
    struct tm *tmptr;
    time_t seconds;

    while ( 1 )
    {
        seconds = time( NULL );
        tmptr = localtime( &seconds );
        printf( "%2d:%02d:%02d ", tmptr->tm_hour, tmptr->tm_min, tmptr->tm_sec );
        show_voltage_current();
        sleep( interval );
    }
}


int main( int argc, char *argv[] )
{
    char filename[ 20 ];

    parse( argc, argv );

    snprintf( filename, 19, "/dev/i2c-%d", i2c_bus );
    handle = open( filename, O_RDWR );
    if ( handle < 0 ) 
    {
        fprintf( stderr, "Error opening bus %d: %s\n", i2c_bus, strerror( errno ) );
        exit( 1 );
    }

    if ( ioctl( handle, I2C_SLAVE, i2c_address ) < 0 ) 
    {
        fprintf( stderr, "Error setting address %02X: %s\n", i2c_address, strerror( errno ) );
        exit( 1 );
    }

    switch ( operation )
    {
        case OP_DUMP:
        {
            show_voltage_current();
            break;
        }

        case OP_VOLTAGE:
        {
            show_voltage();
            break;
        }

        case OP_CURRENT:
        {
            show_current();
            break;
        }

        case OP_MONITOR:
        {
            monitor();
            break;
        }

        default:
        case OP_NONE:
        {
            break;
        }
    }

    close( handle );
    return 0;
}

