The BeagleDrone PowerCape is a power supply cape that uses a TI buck/boost regulator.  It converts a 3V to 14V source into 5V for the BeagleBone and provides reverse polarity and over-voltage protection.  It also sports a TI INA219 power monitor that allows for monitoring of input voltage and current and performs automatic power calculations.

Power to the BeagleBone is controlled by an AVR "supervisor" allowing the BeagleBone to be completely turned off and turned back on due to an external event or an elapsed time.  The regulator and AVR continue to run but consume less than 70uA at 3.7V while in standby.

This repository contains the AVR firmware and any utilities or drivers needed for the PowerCape.  More information can be found at http://andicelabs.com/

