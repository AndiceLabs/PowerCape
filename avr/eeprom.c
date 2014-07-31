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
    eeprom_busy_wait();
}


void eeprom_set_calibration_value( uint8_t value )
{
    eeprom_update_byte( EEPROM_CALIBRATION, value );
}


uint8_t eeprom_get_calibration_value( void )
{
    return eeprom_read_byte( EEPROM_CALIBRATION );
}


uint8_t eeprom_get_board_type( void )
{
    return eeprom_read_byte( EEPROM_BOARD );
}


uint8_t eeprom_get_revision_value( void )
{
    return eeprom_read_byte( EEPROM_REVISION );
}


uint8_t eeprom_get_stepping_value( void )
{
    return eeprom_read_byte( EEPROM_STEPPING );
}


uint8_t eeprom_get_i2c_address( void )
{
    return eeprom_read_byte( EEPROM_I2C_ADDR );
}


uint8_t eeprom_get_charge_current( void )
{
    return eeprom_read_byte( EEPROM_CHG_CURRENT );
}


uint8_t eeprom_get_charge_timer( void )
{
    return eeprom_read_byte( EEPROM_CHG_TIMER );
}
