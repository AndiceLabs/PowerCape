# Meant to be built on a BeagleBone (not cross-compiled)

default: ina219 power

ina219:	ina219.c
	gcc -o ina219 ina219.c

power:	powercape.c
	gcc -o power powercape.c

