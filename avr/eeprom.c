#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include "eeprom.h"


void eeprom_set_bootloader_flag( void )
{
    uint8_t i;
    
    i = eeprom_read_byte( EEPROM_FLAGS );
    i |= EE_FLAG_LOADER;
    eeprom_update_byte( EEPROM_FLAGS, i );
}


void eeprom_set_calibration_value( uint8_t value )
{
    eeprom_update_byte( EEPROM_CALIBRATION, value );
}


uint8_t eeprom_get_calibration_value( void )
{
    return eeprom_read_byte( EEPROM_CALIBRATION );
}
