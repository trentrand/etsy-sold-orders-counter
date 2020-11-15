# etsy orders counter

A desktop screen that displays a live counter of your Etsy store orders!

__Powered by FreeRTOS on a ESP-8266 microcontroller__

## Setup

You can flash your board with the latest version by executing the following commands:

```
$ make
$ make flash
```

Depending on your operating system and connected peripherals, you may need
to change the `SERIAL_PORT` variable in `./Makefile`. To find the path of
your connected microcontroller, try looking at the results of `ls /dev`.

## Debugging

You can view the serial output of your microcontroller by using whichever
program you'd like, just set the baud rate to 115200. This project is setup
to use `picocom`, just invoke the `monitor` target with `make monitor`.
