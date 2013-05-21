BOARD_TAG    = uno
ARDUINO_PORT = /dev/ttyACM*

ARDUINO_LIBS = DuplexFrequencyCounter AdafruitRGBLCDShield LcdMenu Wire Wire/utility

include /usr/share/arduino/Arduino.mk
