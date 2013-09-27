#Colorimeter and nephalometer firmware

With the files in this directory, you can create a colorimeter and/or
nephalometer, if you have the tools.  You find here:

- colorimeter.ino: c++ source for the firmware, to be uploaded to the arduino.
- \*.scad: models for all the parts which should be produced on a 3-D Printer.  Use OpenSCAD to turn them into STLs.
- colorimeter.sch: Schemetic of the electronics.  Use KiCad to open it.  You are expected to solder everything by hand, so there is no board layout.

To compile the firmware, you need the LcdMenu and DuplexFrequencyCounter libraries from our
[other repository](https://github.com/mtu-most/arduino-libraries).

## Required components
Arduino fio
Adafruit lcd shield PCB
3.3 V lcd unit
3 small PCBs
2 LEDs (860 nm for nephalometry and 620 nm for colorimetry)
3 light to frequency encoders
Some wire to connect it all
Some filament to print the casing
Battery
Some nuts and bolts

## Required tools
3-D printer
FTDI (or equivalent) 3.3 V serial communication cable
Soldering iron

## Steps
1. Load the firmware in the Arduino IDE to flash it into the Arduino (or use make upload).
1. Print all the parts
1. Solder the lcd unit to the lcd shield
1. Cut 3 PCBs to size for the slots
1. Solder the components to the PCBs according to the schematic.  Don't solder anything to the ardiuno; use connectors instead.  Each LED goes on a board with a sensor watching it, the third sensor goes on a board of its own, looking away from the board.
1. Put it all together and close it with screws.
