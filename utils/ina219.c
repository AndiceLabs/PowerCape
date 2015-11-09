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

#define REG_CONFIG          0
#define REG_SHUNT           1
#define REG_BUS             2
#define REG_POWER           3
#define REG_CURRENT         4
#define REG_CALIBRATION     5
#define REG_MFG_ID          0xFE
#define REG_DIE_ID          0xFF

#define AVR_ADDRESS         0x21
#define INA_ADDRESS         0x40

typedef enum {
    CHIP_UNKNOWN,
    CHIP_INA219,
    CHIP_INA3221
} chip_type;

chip_type device = CHIP_UNKNOWN;

typedef enum {
    OP_DUMP,
    OP_VOLTAGE,
    OP_CURRENT,
    OP_MONITOR,
    OP_NONE
} op_type;

op_type operation = OP_DUMP;

int channel = 0;
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


chip_type chip_identify( void )
{
    int rc = -1;
    unsigned short reg;
    
    if ( register_read( REG_MFG_ID, &reg ) == 0 )
    {
        if ( reg == 0x5449 )
        {
            if ( register_read( REG_DIE_ID, &reg ) == 0 )
            {
                if ( reg == 0x3220 )
                {
                    rc = CHIP_INA3221;
                }
            }
            else
            {
                rc = CHIP_UNKNOWN;
            }
        }
        else
        {
            rc = CHIP_INA219;
        }
    }
    
    return rc;
}


void show_usage( char *progname )
{
    fprintf( stderr, "Usage: %s <mode> \n", progname );
    fprintf( stderr, "   Mode (required):\n" );
    fprintf( stderr, "      -h --help           Show usage.\n" );
    fprintf( stderr, "      -1 --battery        Battery channel on Solar Cape.\n" );
    fprintf( stderr, "      -2 --solar          Solar channel on Solar Cape.\n" );
    fprintf( stderr, "      -3 --system         System channel on Solar Cape.\n" );
    fprintf( stderr, "      -i --interval       Set interval for monitor mode.\n" );
    fprintf( stderr, "      -w --whole          Show whole numbers only. Useful for scripts.\n" );
    fprintf( stderr, "      -v --voltage        Show channel voltage in mV.\n" );
    fprintf( stderr, "      -c --current        Show channel current in mA.\n" );
    fprintf( stderr, "      -a --address <addr> Override I2C address of power monitor from default of 0x%02X.\n", i2c_address );
    fprintf( stderr, "      -b --bus <i2c bus>  Override I2C bus from default of %d.\n", i2c_bus );
    exit( 1 );
}


void parse( int argc, char *argv[] )
{
    while( 1 )
    {
        static const struct option lopts[] =
        {
            { "battery",    0, 0, '1' },
            { "solar",      0, 0, '2' },
            { "system",     0, 0, '3' },
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

        c = getopt_long( argc, argv, "123a:b:chi:vw", lopts, NULL );

        if( c == -1 )
            break;

        switch( c )
        {
            case '1':
            {
                channel = 0;
                break;
            }

            case '2':
            {
                channel = 1;
                break;
            }

            case '3':
            {
                channel = 2;
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


int get_voltage( int chan, float *mv )
{
    short bus;

    if ( register_read( REG_BUS + ( chan << 1 ), (unsigned short*)&bus ) != 0 )
    {
        return -1;
    }

    if ( device == CHIP_INA219 )
    {
        *mv = ( float )( ( bus & 0xFFF8 ) >> 1 );
    } 
    else 
    {
        *mv = ( float )( bus & 0xFFF8 );
    }
    return 0;
}


int get_current( int chan, float *ma )
{
    short shunt;

    if ( register_read( REG_SHUNT + ( chan << 1 ), &shunt ) != 0 )
    {
        return -1;
    }

    if ( device == CHIP_INA219 )
    {
        *ma = (float)shunt / 10;
    }
    else
    {
        *ma = (float)shunt / 4;
    }
    return 0;
}


void show_current( int chan )
{
    int i;
    float ma;

    if ( device == CHIP_INA219 )
    {
        chan = 0;
    }
    
    if ( get_current( chan, &ma ) )
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


void show_voltage( int chan )
{
    float mv;

    if ( device == CHIP_INA219 )
    {
        chan = 0;
    }
    
    if ( get_voltage( chan, &mv ) )
    {
        fprintf( stderr, "Error reading voltage\n" );
        return;
    }
    printf( "%4.0f\n", mv );
}


void show_voltage_current( int chan )
{
    float mv, ma;

    if ( device == CHIP_INA219 )
    {
        chan = 0;
    }
    
    if ( get_current( chan, &ma ) || get_voltage( chan, &mv ) )
    {
        fprintf( stderr, "Error reading voltage/current\n" );
        return;
    }

    if ( whole_numbers )
    {
        printf( "%4.0fmV  %4.0fmA", mv, ma );
    }
    else
    {
        printf( "%4.0fmV  %4.1fmA", mv, ma );
    }
}


void monitor( void )
{
    int i;
    struct tm *tmptr;
    time_t seconds;

    while ( 1 )
    {
        seconds = time( NULL );
        tmptr = localtime( &seconds );
        printf( "%2d:%02d:%02d ", tmptr->tm_hour, tmptr->tm_min, tmptr->tm_sec );
        if ( device == CHIP_INA219 )
        {
            show_voltage_current( 0 );
            printf( "\n" );
        }
        else
        {
            for ( i = 0; i < 3; i++ )
            {
                printf( " | " );
                show_voltage_current( i );
            }
            printf( "\n" );
        }
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
    
    device = chip_identify();
    if ( device < 0 )
    {
        fprintf( stderr, "Cannot identify power monitor device.\n" );
        exit( 1 );
    }
    else
    {
        switch ( operation )
        {
            case OP_DUMP:
            {
                show_voltage_current( channel );
                printf( "\n" );
                break;
            }

            case OP_VOLTAGE:
            {
                show_voltage( channel );
                break;
            }

            case OP_CURRENT:
            {
                show_current( channel );
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
    }
    
    close( handle );
    return 0;
}

