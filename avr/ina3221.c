/*
 * The MIT License (MIT)
 * 

Copyright (c) 2015 AndiceLabs, LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/


#include <avr/io.h>
#include "bb_i2c.h"
#include "ina3221.h"


static uint8_t ina_reg_read16( uint8_t address, uint8_t reg, uint16_t *data )
{
    uint8_t b[ 4 ];
    
    if ( bb_i2c_write( address, &reg, 1 ) == 0 )
    {
        if ( bb_i2c_read( address, b, 2 ) == 0 )
        {
            *data = ( b[ 0 ] << 8 ) | b[ 1 ];
            return 0;
        }
    }
    return 1;
}


static uint8_t ina_reg_write16( uint8_t address, uint8_t reg, uint16_t data )
{
    uint8_t b[ 4 ];

    b[ 0 ] = reg;
    b[ 1 ] = (( data >> 8 ) & 0xFF );
    b[ 2 ] = ( data & 0xFF );

    if ( bb_i2c_write( address, b, 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}


// Returned value is mV
int16_t ina3221_voltage( uint8_t channel )
{
    int16_t w = 0;
    
    if ( ina_reg_read16( INA_ADDR, INA_REG_BUS + ( channel << 1 ), (uint16_t *)&w ) == 0 )
    {
        w &= 0xFFF8;
    }
    
    return w;
}


#if 0
// Returned value is mA
uint16_t ina219_current( uint8_t channel )
{
    uint8_t addr = INA0_ADDR;
    int16_t w;
    int16_t current = 0;
    
    if ( channel != 0 )
    {
        addr = INA1_ADDR;
    }

    if ( ina_reg_read16( addr, INA_REG_SHUNT, ( uint16_t* )&w ) == 0 )
    {
        current = ( w * 10 ) / 20;
    }
    return current;
}
#endif


uint8_t ina3221_init( void )
{
    // Future initialization / verification
    return 0;
}

