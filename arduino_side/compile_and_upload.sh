#!/bin/bash

arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old .

arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:nano:cpu=atmega328old .
