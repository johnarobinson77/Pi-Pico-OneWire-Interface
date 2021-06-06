/**
 * Copyright (c) 2021 John Robinson.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "OneWire.h"
#include "OneWire.pio.h"

#define ONE_WIRE_FIFODEPTH 4

struct OneWirePIO {
  PIO pio;
  uint offset;
  uint sm;
} owp;

// init_OneWire inits the PIO0 state machine to implement a OneWire interface the pin
// define in ONE_WIRE_GPIO.  Call this fuction after oneWire_Search_Rom().
void init_OneWire(){
   // Choose which PIO instance to use (there are two instances)
    owp.pio = pio0;
    owp.offset = pio_add_program(owp.pio, &OneWire_program);
    owp.sm = pio_claim_unused_sm(owp.pio, true);
    OneWire_program_init(owp.pio, owp.sm, owp.offset, ONE_WIRE_GPIO);
}

// oneWire_reset issues a reset command to the devices on the OneWire bus.
// If wait = true, the function will not return until the command is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_reset(bool wait) {
  if (!wait && pio_sm_is_tx_fifo_full(owp.pio, owp.sm)) {
    return ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE;
  }
  pio_sm_put_blocking(owp.pio, owp.sm, 0x00000002); // issye reset
  return ONE_WIRE_NO_ERROR;
}

// oneWire_wait_for_idle issues a woit for idle bus command
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_wait_for_idle(bool wait){
  if (!wait && pio_sm_is_tx_fifo_full(owp.pio, owp.sm)) {
    return ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE;
  }
  pio_sm_put_blocking(owp.pio, owp.sm, 0x00000000);  // issue wait_for_1
  return ONE_WIRE_NO_ERROR;
}

// oneWire_write_byte writes a single byte to the OneWire bus.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_write_byte(uint8_t data, bool wait) {
  if (!wait && pio_sm_is_tx_fifo_full(owp.pio, owp.sm)) {
    return ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE;
  }
  pio_sm_put_blocking(owp.pio, owp.sm, (data << 6) + (7<<2) +0x03);
  return ONE_WIRE_NO_ERROR;
}

// oneWire_write_uint writes a single unsigned int to the OneWire bus.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_write_uint(uint16_t data, bool wait) {
  if (!wait && pio_sm_is_tx_fifo_full(owp.pio, owp.sm)) {
    return ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE;
  }
  pio_sm_put_blocking(owp.pio, owp.sm, (data << 6) + (15<<2) +0x03);
  return ONE_WIRE_NO_ERROR;
}

// oneWire_push_read_cmd issues a command to read a certain number of bits.  
// The resulting data is placed int the Rx FIFO where it can be read with 
// oneWire_pull_read_data. 
// returns 0 if successful.
// returns error code if number of bits is > 32 or < 1
oneWire_status oneWire_push_read_cmd(uint num_bits) {
  if (num_bits > 32 || num_bits < 1) return ONE_WIRE_ILLEGAL_DATA_SIZE_REQ;
  pio_sm_put_blocking(owp.pio, owp.sm, (num_bits-1 << 2) + 1);  // issue read of num_bits bits
  return ONE_WIRE_NO_ERROR;
}

// oneWire_pull_read_data pulls data from the Rx FIFO that was placed there as a result of
// a call to oneWire_push_read_cmd. num_bits should be the same as the number of bits 
// requested.  This function does not return until there is data in the RX fifo to read.
// So if not preceded by a call to oneWire_push_read_cmd, a hang will result. 
// No CRC check is done.
// returns the data in the fifo.
uint32_t oneWire_pull_read_data(uint num_bits) {
  oneWire_status r = pio_sm_get_blocking(owp.pio, owp.sm);
  return r >> (32-num_bits);
}

// oneWire_read_byte reads one byte of data  No CRC check is performend.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_read_byte (uint8_t *data, bool wait){
  if (!wait && pio_sm_is_tx_fifo_full(owp.pio, owp.sm)) {
    return ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE;
  }
  oneWire_status r = oneWire_push_read_cmd(8);
  if (r != ONE_WIRE_NO_ERROR) return r;
  *data = oneWire_pull_read_data(8) & 0xFF;
}

// oneWire_read_uintreads 1 unsigned int of data   No CRC check is performend.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_read_uint (uint16_t *data, bool wait){
  if (!wait && pio_sm_is_tx_fifo_full(owp.pio, owp.sm)) {
    return ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE;
  }
  oneWire_status r = oneWire_push_read_cmd(16);
  if (r != ONE_WIRE_NO_ERROR) return r;
  *data = oneWire_pull_read_data(16) & 0xFFFF;
}

// oneWire_read_ulong reads one unsigend long of data.  No CRC check is performend.
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returms 0 if successful.
// returns error code it not wait=false and no room in Tx fifo.
oneWire_status oneWire_read_ulong (uint32_t *data, bool wait){
  if (!wait && pio_sm_is_tx_fifo_full(owp.pio, owp.sm)) {
    return ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE;
  }
  oneWire_status r = oneWire_push_read_cmd(32);
  if (r != ONE_WIRE_NO_ERROR) return r;
  *data = oneWire_pull_read_data(32);
}

// Performs the CRC check assuming last byte it the CRC
// return 0 if CRC check is OK
// return error code if check fails.
oneWire_status oneWire_CRC(uint8_t a[], int len) {
  uint8_t crc = 0;
  for (int i = 0;  i < len; i++) {
    uint8_t t = a[i];
    for (int j = 0; j < 8; j++){
      crc ^= t & 1;
      uint8_t pxor = ((crc & 1) != 0) ? 0x18 : 0;
      crc ^= pxor;
      crc = crc >> 1 | crc << 7;
      t >>=  1;
    }
  }
  if (crc != 0) return ONE_WIRE_READ_CRC_FAILURE;
  else return ONE_WIRE_NO_ERROR;
}


// The OneWire PIO state machine takes read requests from 1 to 32 bits.  The read_bytes fuctions
// below convert the requested number of bytes to be read into individual PIO read requests
// minimizing the total number of requests and fifo depth need.

// oneWire_push_read_cmd issues read requests to the PIO state machine to get num bytes
// If wait = true, the function will not return until the data is written to the Tx FIFO.
// returns 0 if successful.
// returns error code if requesting > 16 bytes.
// returns error code if wait = false and there is not enough room in the fifo.
oneWire_status oneWire_push_read_bytes_cmd(int num, bool wait) {
  if (num > 16) return ONE_WIRE_POSSIBLE_FIFO_OVERFLOW;  // a read request of mor than 32 bytes could overflow the fifo
  int num_pushes =num+3;
  if (!wait && num_pushes > 
      (ONE_WIRE_FIFODEPTH - pio_sm_get_tx_fifo_level(owp.pio, owp.sm) * 4)) {
      return ONE_WIRE_NOT_ENOUGH_TX_FIFO_SPACE;  //If not wait andinsufficion fifo space, return error;
  }
  int i;
  for (i = 0;  i <= num-4; i +=  4) {
    pio_sm_put_blocking(owp.pio, owp.sm, (31 << 2) + 1);  // issue read of 32 bits
  }
  int remainder = num - i;
  if (remainder > 0) {
    pio_sm_put_blocking(owp.pio, owp.sm, ((remainder*8-1) << 2) + 1);  // issue read of remainder * 8 bits
  }
  return ONE_WIRE_NO_ERROR;
}

// oneWire_pull_read_bytes pulls num bytes from the rx fifo of the PIO state machine and places them in data[]
// returns 0 if succesfull  It should be paired with a call to oneWire_push_read_cmd() with the same
// number of bytes.
// If wait = true, the function will not return until the data is read from the Rx FIFO.  So if not
// preceeded with oneWire_push_read_bytes_cmd, a hang will occure.
// This function assumes the last byte is a CRC. 
// returns error code if requesting > 16 bytes. No data is read.
// returns error code if wait = false and the data is not already in the RX fifo.  No data is read.
// returns error code if there is a CRC failure on the data that is read.
oneWire_status oneWire_pull_read_bytes(uint8_t data[], int num, bool wait) {
  union {  // easy conversion from long to bytes
    uint8_t  a[4];
    uint32_t l;
  } u;
  if (num > 16) return ONE_WIRE_ILLEGAL_DATA_SIZE_REQ; // read requests limited to 16 bytes
  int num_pullsx4 =((num+3));
  if (!wait && num_pullsx4 > pio_sm_get_rx_fifo_level(owp.pio, owp.sm) * 4) { // is the data there?
      return ONE_WIRE_NOT_ENOUGH_DATA_IN_RX_FIFO; 
  }
  int i;
  for (i = 0;  i <= num-4; i +=  4) {
      u.l = pio_sm_get_blocking(owp.pio, owp.sm);
      for (int k = 0;  k < 4; k++) data[i+k] = u.a[k];
  }
  int remainder = (num - i);
  if (remainder > 0) {
      u.l = pio_sm_get_blocking(owp.pio, owp.sm);
      u.l >>= (32-(remainder*8));
      for (int k = 0;  k < remainder; k++) data[i+k] = u.a[k];
  }
  return oneWire_CRC(data, num);
}

// one_wire_read_bytes() reads num bytes from the device and places them in data[].
// returns 0 if successful;
// returns error code if request size is > 16 bytes.
// returns error code if there was a CRC error.
oneWire_status oneWire_read_bytes(uint8_t data[], int num) {
  oneWire_status r = oneWire_push_read_bytes_cmd(num, true);
  if (r != ONE_WIRE_NO_ERROR) return r;
  return oneWire_pull_read_bytes(data, num, true);
}

// The next set of functions control oneWire interface by
// direct manipulation of the GPIO pins and so cannot beu sed after 
// the PIO has been initialized.  There spacific use if for 
// search rom functions

// Timing Constants
#define ONE_WIRE_RESET_PULSE 500
#define ONE_WIRE_PRESENCE_WAIT 500
#define ONE_WIRE_PRESENCE_CT 30
#define ONE_WIRE_WRITE_0 60
#define ONE_WIRE_POST_WRITE_0 5
#define ONE_WIRE_WRITE_1 5 
#define ONE_WIRE_POST_WRITE_1 60
#define ONE_WIRE_READ_PULSE 4
#define ONE_WIRE_READ_SAMPLE 8
#define ONE_WIRE_POST_READ 53

static void init_OneWireBB(){
  // init GPIO pin to tristate out but output of 0
  gpio_init(ONE_WIRE_GPIO);
  gpio_set_dir(ONE_WIRE_GPIO, GPIO_IN);
  gpio_put(ONE_WIRE_GPIO, 0);
  //gpio_set_input_enabled(ONE_WIRE_GPIO, true);
}

// reset and check for presence
static bool oneWire_resetBB() {
  // drive out the reset pulse
  gpio_set_dir(ONE_WIRE_GPIO, GPIO_OUT);
  busy_wait_us_32(ONE_WIRE_RESET_PULSE);
  gpio_set_dir(ONE_WIRE_GPIO, GPIO_IN);
  // wait for one cycle time
  busy_wait_us_32(ONE_WIRE_PRESENCE_CT);
  // poll the input util a low signal is seen
  // and set found to true;
  bool found = false;
  int i;
  for (i = 0;  i < 8; i++){
    if (false == gpio_get(ONE_WIRE_GPIO)) {
      found = true;
      break;
    }
    busy_wait_us_32(ONE_WIRE_PRESENCE_CT);
  }
  busy_wait_us_32(ONE_WIRE_PRESENCE_WAIT - ONE_WIRE_PRESENCE_CT * (8-i));
  return found;
}

static bool oneWire_write_byteBB(uint8_t data){
  for (int b = 1;  b <= 128; b <<= 1) {
    if ((b & data) != 0 ) {
      // write 1 sequence
      gpio_set_dir(ONE_WIRE_GPIO, GPIO_OUT);
      busy_wait_us_32(ONE_WIRE_WRITE_1);
      gpio_set_dir(ONE_WIRE_GPIO, GPIO_IN);
      busy_wait_us_32(ONE_WIRE_POST_WRITE_1);
    } else {
      // write 0 sequence
      gpio_set_dir(ONE_WIRE_GPIO, GPIO_OUT);
      busy_wait_us_32(ONE_WIRE_WRITE_0);
      gpio_set_dir(ONE_WIRE_GPIO, GPIO_IN);
      busy_wait_us_32(ONE_WIRE_POST_WRITE_0);
    }
  }
  return true;
}


static bool oneWire_read_bitBB() {

    gpio_set_dir(ONE_WIRE_GPIO, GPIO_OUT);
    busy_wait_us_32(ONE_WIRE_READ_PULSE);
    gpio_set_dir(ONE_WIRE_GPIO, GPIO_IN);
    busy_wait_us_32(ONE_WIRE_READ_SAMPLE);
    bool bit = gpio_get(ONE_WIRE_GPIO);
    busy_wait_us_32(ONE_WIRE_POST_READ);
    /*
    gpio_set_dir(ONE_WIRE_GPIO, GPIO_OUT);
    bool bit = gpio_get(ONE_WIRE_GPIO);
    gpio_set_dir(ONE_WIRE_GPIO, GPIO_IN);
*/
    return bit;
}

static void oneWire_write_bitBB(bool bit) {
    
    if (bit) {
        // write 1 sequence
        gpio_set_dir(ONE_WIRE_GPIO, GPIO_OUT);
        busy_wait_us_32(ONE_WIRE_WRITE_1);
        gpio_set_dir(ONE_WIRE_GPIO, GPIO_IN);
        busy_wait_us_32(ONE_WIRE_POST_WRITE_1);
    }
    else {
        // write 0 sequence
        gpio_set_dir(ONE_WIRE_GPIO, GPIO_OUT);
        busy_wait_us_32(ONE_WIRE_WRITE_0);
        gpio_set_dir(ONE_WIRE_GPIO, GPIO_IN);
        busy_wait_us_32(ONE_WIRE_POST_WRITE_0);
    }
}

// oneWire_search_rom searches all the devices on the one wire bus and collects
// the roms for for all the devices.  The roms will be put in the devs array.
// The pointer to array passed in must be to one that is big enough to handle 
// the maximum number of devices on the bus.  
// If a call to this function is needed it must be done before init_OneWire(). 
// returns the number of devices it wrote to the devs array if successful.
// returns error code if a failure occured.
int oneWire_search_rom(uint64_t devs[]) {
    init_OneWireBB();
    int nextdev = 0;
    uint64_t current = 0;
    uint64_t discrepancy = 0;
    bool done = false;  busy_wait_us_32(100);
    while (!done) {
        int bit;
        if (!oneWire_resetBB()) return 0; // no devices on the bus.
        oneWire_write_byteBB(0xF0); // search rom command
        for (bit = 0; bit < 64; bit++) {
            bool wo1 = oneWire_read_bitBB();
            bool wo2 = oneWire_read_bitBB();
            if (wo1 ^ wo2) { //no discrepancy
                if (wo1) current |= 1ULL << bit;
                else current &= ~(1ULL << bit);
                discrepancy &= ~(1ULL << bit);
                oneWire_write_bitBB(wo1);
            } else if (wo1 == false && wo2 == false) {
                if ((discrepancy & (1ULL << bit)) != 0) {  // was a discrepancy last pass
                    oneWire_write_bitBB((current & (1ULL << bit)) != 0);
                } else {
                    current |= 1ULL << bit;
                    oneWire_write_bitBB(true);
                    discrepancy |= (1ULL << bit);
                }
            } else {
                return ONE_WIRE_SEARCH_ROM_FAILURE;  // some kind of error happened.
            }
        }
        // save off the current rom
        devs[nextdev] = current;
        nextdev++;
        //deal with discrepancy
        for (bit = 63; bit >= 0; bit--) {  
            if ((discrepancy & (1ULL << bit)) != 0) {  // if is a discrepancy
                if ((current & (1ULL << bit)) != 0) { // if current is 1
                    current &= ~(1ULL << bit); // set to 0 for next pass
                    break; // done dealing with descrepancies
                } else {
                    discrepancy &= ~(1ULL << bit); // clear the descrepancy
                }
            }
        }
        if (bit < 0) { // all descrpancies cleared so we're done
            done = true;
        }
    }
    return nextdev;
}
