# Wireless Nintendo 64 Controller Project

# Overview

This page describes my project to make a Nintendo 64 controller wireless. 
You plug your favourite controller into the transmitter, then plug the receiver into the console.

The system is built on Arduino/AVR and utilises the popular RFM69 radio modules for wireless communication.

## Hardware

The transmitter (what you plug the controller into) consists of:

* 3.3V Arduino Pro Mini, the brains of the operation
* Sparkfun RFM69 Breakout, to enable wireless communication
* Breadboard, breadboard power supply and 7.4V battery.
* Half a N64 controller extension cord (the end that connects to a controller)

The receiver (what plugs into the console) consists of:

* Another 3.3V Arduino Pro Mini
* Another Sparkfun RFM69 Breakout
* ATtiny85 dedicated to handling communication with the console
* Breadboard.
* The other half of the N64 controller extension cord (the end that connects to the console)

## Software

Transmitter software is located in the **wireless_n64_controller** sketch.
Flash **wireless_n64_controller.ino** to the Arduino Pro Mini on the transmitter.

Receiver software is located in the **wireless_n64_receiver** sketch.
Flash **wireless_n64_receiver.ino** to the Arduino Pro Mini on the receiver.
Flash **ATtiny85-n64-controller.ino** to the ATtiny85 on the receiver.

## Breadboard set up: Transmitter (Connect to controller)

![Transmitter](fritzing/transmitter_bb.png?raw=True)

## Breadboard set up: Receiver (Connect to console)

![Transmitter](fritzing/receiver_bb.png?raw=True)

