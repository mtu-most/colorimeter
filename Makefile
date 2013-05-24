BOARD_TAG    = fio
ARDUINO_PORT = /dev/ttyUSB*

ARDUINO_LIBS = DuplexFrequencyCounter AdafruitRGBLCDShield LcdMenu Wire Wire/utility EEPROM

include /usr/share/arduino/Arduino.mk
