arduino-caravelle
=========================

This code is just for display. The required libraries are not included.

kitchen.ino
----
Arduino code for controlling refridgerator and water pump in the "kitchen" part of the car.

The temperature sensors are the Dallas DS20S18 1-wire sensor and are connected to the same bus with their own unique address.

lcd.ino
----
Arduino code for the Nokia 5110 LCD module. This is connected to the "kitchen"-Arduino and displays the information and status of the refridgerator and water pump.

The communication is done over serial port and a simple protocol is used to send status updates.

The images shown on the display are simple 2-dimensional bit-arrays stored in the program memory.
