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


#ifndef __INA3221_H__
#define __INA3221_H__

#define INA_ADDR        ( 0x40 << 1 )

#define INA_REG_CONFIG  0x00
#define INA_REG_SHUNT   0x01    // shunt voltage
#define INA_REG_BUS     0x02    // bus voltage

#define INA_CHAN_BATT   0
#define INA_CHAN_SOLAR  1
#define INA_CHAN_BONE   2


int16_t ina3221_voltage( uint8_t channel );
//uint16_t ina3221_current( uint8_t channel );
uint8_t ina3221_init( void );


#endif  // _INA3221_H__
