#ifndef __EEPROM_H__
#define __EEPROM_H__

#define EEPROM_FLAGS        ( (uint8_t*)0 )
#define EEPROM_CALIBRATION  ( (uint8_t*)1 )

#define EE_FLAG_LOADER      0x01

void eeprom_set_bootloader_flag( void );
void eeprom_set_calibration_value( uint8_t value );
uint8_t eeprom_get_calibration_value( void );

#endif  // __EEPROM_H__
