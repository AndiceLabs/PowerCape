#ifndef __EEPROM_H__
#define __EEPROM_H__

#define EEPROM_FLAGS        ( (uint8_t*)0 )
#define EEPROM_CALIBRATION  ( (uint8_t*)1 )
#define EEPROM_BOARD        ( (uint8_t*)2 )
#define EEPROM_REVISION     ( (uint8_t*)3 )
#define EEPROM_STEPPING     ( (uint8_t*)4 )
#define EEPROM_I2C_ADDR     ( (uint8_t*)5 )
#define EEPROM_CHG_CURRENT  ( (uint8_t*)6 )
#define EEPROM_CHG_TIMER    ( (uint8_t*)7 )

#define EE_FLAG_LOADER      0x01

void eeprom_set_bootloader_flag( void );
void eeprom_set_calibration_value( uint8_t value );
uint8_t eeprom_get_calibration_value( void );
uint8_t eeprom_get_board_type( void );
uint8_t eeprom_get_revision_value( void );
uint8_t eeprom_get_stepping_value( void );
uint8_t eeprom_get_i2c_address( void );
uint8_t eeprom_get_charge_current( void );
uint8_t eeprom_get_charge_timer( void );

#endif  // __EEPROM_H__
