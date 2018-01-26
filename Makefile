BOARD_TAG    = mega2560
ARDUINO_PORT = /dev/ttyACM*

ARDUINO_LIBS = DuplexFrequencyCounter AdafruitRGBLCDShield LcdMenu Wire Wire/utility EEPROM

include /usr/share/arduino/Arduino.mk
