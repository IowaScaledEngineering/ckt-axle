#*************************************************************************
#Title:    Makefile
#Authors:  Michael Petersen
#          
#*************************************************************************

BASE_NAME = ard-axlesim

BOARD  = arduino:avr:uno

ARDUINO = arduino-cli

help:
	@echo "make verify..... Build $(BASE_NAME)"
	@echo "make upload..... Flash the firmware"
	@echo "make terminal... Open uart terminal"

verify:
	$(ARDUINO) --verify --board $(BOARD) $(BASE_NAME).ino

upload:
	$(ARDUINO) compile -v -b $(BOARD) $(BASE_NAME).ino
	$(ARDUINO) upload -v -b $(BOARD) -p /dev/ttyUSB0 $(BASE_NAME).ino

terminal:
	miniterm.py --filter=direct /dev/ttyUSB0 115200
