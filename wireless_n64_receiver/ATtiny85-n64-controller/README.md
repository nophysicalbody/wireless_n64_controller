# n64_if_firmware
Firmware for the "Build Your Own Nintento 64 Controller" board

This device is to act as a virtual Nintendo 64 controller when connected to a Nintendo 64 console.
Instead of having physical buttons and a joystick, it has an external software interface which can be connected to an arduino or any microcontroller. This allows makers to build their own custom nintendo controller, be it with a new shape, or with the capability to perform scripted button/joystick sequences.

This code is written in C and assembly. It depends on the #Arduino library and is designed specifically for the ATtiny85.

## Interface design
### Interface to N64 console
Protocol: Proprietary Nintendo 64 serial protocol.

![N64-timing-diagram](Images/N64-timing-diagram.png?raw=True)

Goal: To respond to commands from the Nintendo 64 console
Building on the great reverse-engineering done by Pieter-Jan and qwerty-modo (links below) I have developed a bit-bashing implementation of the protocol in assembly on the ATtiny85.

http://www.qwertymodo.com/hardware-projects/n64/n64-controller
http://www.pieter-jan.com/node/10

The Nintendo 64 console sends commands to the controller, which can be polls for controller state and controller configuration (currently supported) or to read/write the controller's memory pack (currently not supported). 

Button and joystick states are polled for, by the console. The console polls the controller at roughly 25 Hz during normal gameplay (At least, as measured by me while playing Diddy Kong Racing). The console also periodically requests a status update from the controller, which the controller uses to indicate things such as whether there is a rumble pack or memory pack inserted.

During console startup and transition points in the game's program, these requests seem to speed up/slow down, leading to brief periods of much higher/random frequencies.

The interface is handled via interrupts written in inline assembly. Getting C variables in and out of an assembly routine is cryptic at the best of times, so for anyone trying to build on what I created refer to this guide to make interpreting it easier:
https://www.nongnu.org/avr-libc/user-manual/inline_asm.html

### Interface to another microcontroller
Protocol: SPI "inspired" protocol, specifically designed to accommodate clock-stretching.

Goal: To provide a simple and reliable way to interface with the AVR n64 controller to set button and joystick states.

Not claiming it's a good implementation, but it's mine!

#### Why I didn't use serial

Initially I started using standard TTL serial. This worked, but not reliably.

Commands from the console are handled with interrupts, which can take up to ~160us to run. If a console command happens to co-incide with an asynchronous serial TTL transmission (as used by the standard Arduino Serial or SoftwareSerial libraries), this delay de-synchronises the sender and receiver, leading to corruption of the incoming packets.

I initially tried to work around this using a CTS (clear to send) I/O line. By assuming the 25Hz update frequency, I could time pulses of CTS to occur immediately after the command was processed. The reliability improved a bit, but was still far from iron-clad as at times the commands from the console would come faster than expected (for instance, during console start up) and still lead to corruption.

To solve this problem I moved to a synchronous protocol like SPI, which has a dedicated clock line.
The clock line is controlled by the ATtiny chip, so if required it can hold the clock in the current state while it services the N64 console command routine without interrupting the transmission of virtual controller status change requests from the external device.

Below is a capture from my test-bench, which shows an interrupted transmission.

![clock-stretch-annotated](Images/clock-stretch-annotated.png?raw=True)

You can clearly see how the clock line pauses while the ATtiny chip services a poll request from the N64 console. This pause can be detected by the sender (which controls the Data line, shown in blue above) so the participants can remain in sync and the data is not corrupted.

## Setup

For this guide, I will be using the dedicated ATtiny programmer.

Before following the instructions for setting fuses or uploading the program below, you need to ensure you have the following set up:

* An AVR programmer (in my case the ATiny programmer) installed and set up on your PC.
* The Arduino IDE installed on your PC
* The ATtiny board installed in your Arduino IDE.

The Sparkfun hookup guide covers all of this so I will not repeat the instructions here.
https://learn.sparkfun.com/tutorials/tiny-avr-programmer-hookup-guide

If you are one of those cool folks using Ubuntu/Linux, the process is slightly different. Refer to the excellent guide here:
https://www.sigmdel.ca/michel/program/avr/sparkfun_tiny_programmer_en.html

### Fuses

Hook up your ATtiny to your programmer, and connect the programmer to your PC.

From the command line run the following command. This will confirm whether the ATtiny and the programmer are hooked up correctly.

```
~$ avrdude -c usbtiny -p attiny85
avrdude: AVR device initialized and ready to accept instructions
Reading | ################################################## | 100% 0.01s
avrdude: Device signature = 0x1e930b (probably t85)
avrdude: safemode: Fuses OK (E:FF, H:DF, L:62)
avrdude done.  Thank you.
```

We need to set the fuses of the ATtiny85 so it operates using the internal oscillator and does not divide the system clock by 8 - meaning it runs at 8MHz instead of 1MHz. To do this, run the below command:

```
~$ avrdude -c usbtiny -p attiny85 -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0xFF:m
avrdude: AVR device initialized and ready to accept instructions
...
... many lines later ...
...
avrdude: safemode: Fuses OK (E:FF, H:DF, L:E2)
avrdude done.  Thank you.
```

### Upload program

To compile and upload the firmware, use the Arduino IDE.

Open the `n64_if/n64_if_firmware/ATtiny85-n64-controller/ATtiny85-n64-controller.ino` file in the Arduino IDE.

Select the board type as `ATtiny25/45/85` under `Tools -> Board -> ATtiny Microcontrollers.`

Select the processor as `ATtiny85` under `Tools -> Processor`.

Select the clock as `Internal 8MHz` under `Tools -> Clock`.

Select the programmer as `USBtinyISP` under `Tools -> Programmer`

With this done, compile and upload the program by pressing the upload button `(Ctrl+U)`

## ATtiny85 pinout

![Pinout](Images/PinoutDiagram.png?raw=True)



## Reference

The giants whose shoulders I stood on while building this:

http://www.pieter-jan.com/node/10

http://www.qwertymodo.com/hardware-projects/n64/n64-controller





