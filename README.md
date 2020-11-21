# etsy orders counter

A desktop screen that displays a live counter of your Etsy store orders!

__Powered by FreeRTOS on a ESP-8266 microcontroller__

## Setup

You should follow standard instructions in the
[esp-open-rtos README](https://github.com/SuperHouse/esp-open-rtos/blob/master/README.md)
to ensure your environment has the dependencies necessary to build the project.

Next you must setup your secrets in the `./config.h` file. See instructions in
`./config.example.h` to get started.

Once you've setup your secrets, you can flash your board with this project's
firmware by executing the following commands in your terminal:

```
$ make rebuild
$ make flash
```

Depending on your operating system and connected peripherals, you may need
to change the `SERIAL_PORT` variable in `./Makefile`. To find the path of
your connected microcontroller, try looking at the results of `ls /dev`.

### Supported build environments

This project is developed within a virtualized instance of Ubuntu.

The Makefile also includes a utility target `dev` which will step your
shell into a containerized image which has the necessary dependency for
esp-open-rtos. On MacOSX, because of limitations with Docker, you can not
mount usb devices to a docker container. This means that utility targets
like `flash` and `erase_flash` must be invoked outside of the container.

## Debugging

You can view the serial output of your microcontroller by using whichever
program you'd like, just set the baud rate to 115200. This project is setup
to use `picocom`, just invoke the `monitor` target with `make monitor`.
