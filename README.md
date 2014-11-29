The PowerCape is a power supply cape for the BeagleBone that combines a TI
buck/boost regulator with a Li-Ion/Li-Poly charging circuit.  It converts a 3V
to 14V DC source or 3.7V battery source into 5V for the BeagleBone.  It also
sports a TI INA219 power monitor that allows for monitoring of the battery
voltage and current.

Power to the BeagleBone is controlled by an AVR "supervisor" allowing the
BeagleBone to be completely powered off and turned back on due to an external
event or an elapsed time.  The regulator and AVR continue to run but consume
less than 100uA while running from a 3.7V battery source.  Quiescent current
increases to 1.2mA when using the DC power input.

This repository contains the AVR firmware and utilities for the PowerCape.

More information can be found at http://andicelabs.com/

