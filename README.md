# OneWire Interface Using PIO in the Pi Pico

The software posted here implements a OneWire or Dallas interface on the Raspberry Pi Pico using one PIO state machine. It is written in C. It provides a means to both initiate transactions and wait for the response or poll the interface for when it is ready to take another transaction. The latter can be used to let the processor perform other functions while waiting for the interface to become ready.

# Restrictions

## Search Rom

The first thing to note here is that the OneWire PIO state machine only implements the reads and writes to the OneWire PIO state machine initialization. The search rom function is not part of the PIO state machine. (Search rom is the function that identifies the device code and serial numbers of all the devices on the OneWire bus.) The search rom function is complicated and would be difficult to put into a PIO state machine. So, the search rom function provided here is done with the processor using bit banging of the GPIO pin driving the interface wire. It is normally run only one once before any data transactions.

For the above reason the search rom function must be run before the PIO state machine is initialized so that the processor can still directly control the GPIO.# Pi-Pico-OneWire-Interface

## Posting Read Commands

The PIO interface allows you to post read commands to the Tx FIFO, go off and do other things and then come back to read the data from the Rx FIFO. The FIFOs are limited in size so posting too many read commands without reading the resulting data from the Rx FIFO can lead to a hang. Total outstanding reads should be limited to 4 read requests of less than 4 bytes each or 1 read request of 16 bytes before reading the resulting data.

## Long Operations

In some cases, a command to a OneWire device will take a long time to complete and often, that device will pull down on the bus until that transaction is complete. An example is the DS18B20 thermal sensor device when issuing the thermal conversion command. While thermal conversion is taking place the DS18 pulls the bus to 0 until the operation is complete.

The PIO interface does not natively know that such an operation is taking place and if you start another read or write command, it will start right away without waiting. To work around this, there is a wait for idle command that will stall the PIO interface until the bus goes high. Issuing a wait for idle after sending such a command will hold off any future transactions until the bus returns to a 1.

Of special note is that the reset command in which the device may pull down on the bus for a long time. But the PIO interface finishes the reset command with the wait for idle, so no special handling is required for a reset command.

# Files

The OneWire Interface takes place in 3 main files.

**OneWire.pio** defines the state machine and the initialization function that initializes the interface.

**OneWire.c** defines a number of functions for sending data to and from the PIO state machine. It also contains the search rom function.

**OneWire.h** declares all the public functions and has some #defines of error codes. The documentation of the functions can be found there.

Also included in this post are the following two files.

**DS1820B.c** Is a program that uses the OneWire interface to talk to multiple DS18B20 thermal sensor chips. It is provided as an example of how to use the OneWire interface. However, it references a separate library for displaying the temperatures on a small display driven by a SH1107 chip over SPI that is not important to using the one wire interface. Any calls to functions with a &quot;srn\_&quot; prefix can be removed or replaced with some other display mechanism as can any reference to blink or LED functions.

**CMakeList.txt** is used to build the temp.uf2 file sent to the Pico. Again, all that is required for use of the OneWire interface code OneWire.pio and OneWire .c. The rest of the files should be replaced with your program files. The reset is for display and debug.

# Picture

Here is a picture of the temp program running on a tiny2040 board. It is reading the temperature about every minute or so and displaying the temperatures in the left-hand column. It is also counting the number of CRC passes and fails found on the bus. Across the top is a bar graph of the last 64 temperature readings.
