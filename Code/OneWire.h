/**
 * Copyright (c) 2021 John Robinson.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ONE_WIRE_H
#define ONE_WIRE_H

#define ONE_WIRE_GPIO 7

typedef uint16_t oneWire_status;

// oneWire_search_rom searches all the devices on the one wire bus and collects
// the roms for for all the devices.  The roms will be put in the devs array.
// The pointer to array passed in must be to one that is big enough to handle 
// the maximum number of devices on the bus.
// If a call to this function is needed it must be done before init_OneWire().  
// returns the number of devices it wrote to the devs array if successful.
// returns error code if a failure occured.
int oneWire_search_rom(uint64_t devs[]);

// init_OneWire inits the PIO0 state machine to implement a OneWire interface the pin
// define in ONE_WIRE_GPIO.  Call this fuction after oneWire_Search_Rom().
void init_OneWire();

// oneWire_reset issues a reset command to the devices on the OneWire bus.
// If wait = true, the function will not return until the command is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_reset(bool wait);

// oneWire_wait_for_idle issues a woit for idle bus command
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_wait_for_idle(bool wait);

// oneWire_write_byte writes a single byte tp the onewire bus.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_write_byte(uint8_t, bool wait);

// oneWire_write_uint writes a single unsigned int to the OneWire bus.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_write_uint(uint16_t data, bool wait);

// oneWire_push_read_cmd issues a command to read a certain number of bits.  
// The resulting data is placed int the Rx FIFO where it can be read with 
// oneWire_pull_read_data. 
// returns 0 if successful.
// returns error code if number of bits is > 32 or < 1
oneWire_status oneWire_push_read_cmd(uint num_bits);

// oneWire_pull_read_data pulls data from the Rx FIFO that was placed there as a result of
// a call to oneWire_push_read_cmd. num_bits should be the same as the number of bits 
// requested.  This function does not return until there is data in the RX fifo to read.
// So if not preceded by a call to oneWire_push_read_cmd, a hang will result. 
// No CRC check is done.
// returns the data in the fifo.
uint32_t oneWire_pull_read_data(uint num_bits);

// oneWire_read_byte reads one byte of data  No CRC check is performend.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_read_byte (uint8_t *data, bool wait);

// oneWire_read_uintreads 1 unsigned int of data   No CRC check is performend.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_read_uint (uint16_t *data, bool wait);

// oneWire_read_ulong reads one unsigend long of data.  No CRC check is performend.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_read_ulong (uint32_t *data, bool wait);

// Performs the CRC check assuming last byte it the CRC
// return 0 if CRC check is OK
// return error code if check fails.
oneWire_status oneWire_CRC(uint8_t a[], int len);

// The OneWire PIO state machine takes read requests from 1 to 32 bits.  The read_bytes fuctions
// below convert the requested number of bytes to be read into individual PIO read requests
// minimizing the total number of requests and fifo depth need.

// oneWire_push_read_cmd issues read requests to the PIO state machine to get num bytes
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returns 0 if successful.
// returns error code if requesting > 16 bytes.
// returns error code if wait = false and there is not enough room in the fifo.
oneWire_status oneWire_push_read_bytes_cmd(int num, bool wait);

// oneWire_pull_read_bytes pulls num bytes from the rx fifo of the PIO state machine and places them in data[]
// returns 0 if succesfull  It should be paired with a call to oneWire_push_read_cmd() with the same
// number of bytes.
// If wait = true, the function will not return until the data is read from the Rx FIFO.  So if not
// preceeded with oneWire_push_read_bytes_cmd, a hang will occure.
// This function assumes the last byte is a CRC. 
// returns error code if requesting > 16 bytes. No data is read.
// returns error code if wait = false and the data is not already in the RX fifo.  No data is read.
// returns error code if there is a CRC failure on the data that is read.
oneWire_status oneWire_pull_read_bytes(uint8_t data[], int num, bool wait);

// one_wire_read_bytes() reads num bytes from the device and places them in data[].
// returns 0 if successful;
// returns error code if request size is > 16 bytes.
// returns error code if there was a CRC error.
oneWire_status oneWire_read_bytes(uint8_t data[], int num);


// error codes
#define ONE_WIRE_NO_ERROR 0
#define ONE_WIRE_POSSIBLE_FIFO_OVERFLOW -1
#define ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE -2
#define ONE_WIRE_NOT_ENOUGH_DATA_IN_RX_FIFO -3
#define ONE_WIRE_READ_CRC_FAILURE -4
#define ONE_WIRE_SEARCH_ROM_FAILURE -5
#define ONE_WIRE_ILLEGAL_DATA_SIZE_REQ -6

#endif //ONE_WIRE_H
