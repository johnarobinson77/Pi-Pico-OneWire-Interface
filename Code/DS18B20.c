/**
 * Copyright (c) 2021 John Robinson.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/timer.h"
#include "OneWire.h"
#include "../../Display/sh1107/blink.h"
#include "../../Display/sh1107/Display_all.h"


typedef struct DS18B20dev {
  uint16_t family_code;
  uint16_t rom_crc;
  uint64_t serial_num;
  uint16_t temperature;
  uint8_t alarm_th;
  uint8_t alarm_tl;
  uint8_t config;
} DS18B20dev_t;

#define DEBUG
#ifdef DEBUG
//set up a text screen region
char_screen_region_t csrd;
char txt[32];

void print_d(char *ps){
  if (csrd.ccol_rgt != 15 || csrd.crow_bot != 15) {
    init_char_screen_region(&csrd, 0,0,15,15);
  }
  srn_print(&csrd, ps);
}

#endif



// Usefull to get the seruial number if there is only
// one DS18B20 on the bus.  If there is a CRC failure or
// the familily code is not 0x28, the family code on the
// struct is set to 0 and returns false.
bool get_DS18_rom(DS18B20dev_t *dev){
  union {
    uint8_t a[8];
    uint64_t d;
  } u;
  // send the read rom code
  oneWire_write_byte(0x33, true);
  // read back 8 bytes
  oneWire_status stat = oneWire_read_bytes(u.a, 8);
  if (stat != ONE_WIRE_NO_ERROR || u.a[0] != 0x28) {
    dev->family_code = 0; // invalidate the device
    return false;
  } else {
    dev->family_code = 0x28;
  }
  dev->serial_num = (u.d >> 8) & 0xFFFFFFFFFFFF;
  dev->rom_crc = u.a[7];
  
  return true;
}

void send_DS18_match_rom(DS18B20dev_t *dev) {
  union {
    uint8_t a[8];
    uint64_t d;
  } u;
  // put the rom data together
  u.d = dev->serial_num << 8;
  u.a[0] = dev->family_code;
  u.a[7] = dev->rom_crc;
  // send the match rom command
  oneWire_write_byte(0x55, true);
  for (int i = 0;  i < 8; i+=2){
    oneWire_write_uint(((uint16_t)u.a[i+1]<<8) +u.a[i],true);
  }
  //sprintf(txt, "\n%016llx", u.d);
  //print_d(txt);
}

void send_DS18_skip_rom(){
  // send the skip rom command
  oneWire_write_byte(0xCC, true);
}

int search_DS18_rom(DS18B20dev_t *devs[]) {
  uint64_t roms[16];
  int num_roms = oneWire_search_rom(roms);
  for (int i = 0;  i < num_roms; i++) {
    devs[i] = (DS18B20dev_t*)malloc(sizeof(DS18B20dev_t));
    devs[i]->family_code = roms[i] & 0xFF;
    devs[i]->serial_num = roms[i] >> 8 & 0xFFFFFFFFFFFF;
    devs[i]->rom_crc = roms[i] >> 56 & 0xFF;
  }
  return num_roms;
}

bool get_DS18_scratch(DS18B20dev_t *dev) {

  // send match rom to identify device
  send_DS18_match_rom(dev);
  union {
    uint8_t  a[12];
    uint32_t l[3];
  } u;
  // send the read scratch command
  oneWire_write_byte(0xBE, true);
 
  // read back 9 bytes
  oneWire_status stat = oneWire_read_bytes(u.a, 9);
  if (stat != ONE_WIRE_NO_ERROR) return false;

  // store the scratch data in the dev struct
  dev->temperature = (u.a[1] << 8) | u.a[0];
  dev->alarm_th = u.a[2];
  dev->alarm_tl = u.a[3];
  dev->config   = u.a[4];

  return true;
}

/*
// checks to see if the device is done with whatever
static inline bool are_all_DS18_done() {
  return oneWire_wire_state();
}
*/

// Tells all devices to start the temperature conversion
// if conversion didn't start returns false.
// if wait is  true, the function will wait until
// the temperature converion is comple. Returns false if timeout
bool convert_DS18_temp(bool wait) {
  // send the skip rom command
  send_DS18_skip_rom();
  // send the convert temp command
  oneWire_write_byte(0x44, true);
  // wait a few microseconds to see if convertions started
  busy_wait_us_32(100);
  oneWire_wait_for_idle(true);
 
  return true;
}


int main() {
    // standrd init call for RP2040
  stdio_init_all();
  // this init sets up the SPI interface to the display and ends with a clear screen
  init_sh1107_SPI();
  // this inits the GPIO that drave the RGB LED on the tiny2040 board and is not 
  // required for the display.  Only here for code debug purposes
  init_tiny2040_leds();

  
// start a rom search to find all devices on the bus
  DS18B20dev_t *devs[10];
  int num_devs = search_DS18_rom(devs);
  if (num_devs == 0) {
    print_d("\nNo device responded");
    start_blinking(true, false, false, 20000);
  }
  if (num_devs == ONE_WIRE_SEARCH_ROM_FAILURE || num_devs  >= 10) {
    print_d("\nsearch_rom failed");
    start_blinking(true, false, false, 20000);
  }
  // print out devices
  for (int i = 0;  i < num_devs; i++) {
    sprintf(txt, "\n%d DC = %02X", i, devs[i]->family_code);
    print_d(txt);
    sprintf(txt, "\n%012llX", devs[i]->serial_num);
    print_d(txt);
  }
  start_blinking(true, false, true, 1);

  init_OneWire();  // start up the PIO state machine

  // two character screen regions
  char_screen_region_t csr1;
  char_screen_region_t csr2;
  // two graphics screen regions are used for various
  graph_screen_region_t gsr;
  graph_screen_region_t gsras;

  // set up two character regions side by side on the bottom of the screen.
  // left one for values, right one for refresh time.
  // set up the whole text box and write the text header
  init_char_screen_region(&csr1, 0, 9,  7, 15);
  srn_print(&csr1, "Deg C");
  // reset the text box to exlude the header so it doesn't get cleared
  init_char_screen_region(&csr1, 0, 10,  8, 15);
  
  // set up the whole text box and write the text header
  init_char_screen_region(&csr2, 8, 9, 15, 15);
  srn_print(&csr2, "Errors");
  // reset the text box to exlude the header so it doesn't get cleared
  init_char_screen_region(&csr2, 9, 10, 15, 15);
  
  // set up a graphics region on the top half of the screen
  map_window(&gsr, -2.0, 1.0, 2.0, -1.0, 0, 0, 127, 63);
  map_autoscroll_bar_window(&gsras, 50.0, 20.0, 0, 0, 127, 63);
  srn_refresh(); 

  // counts of CRC errors
  int p = 0;
  int f = 0;

while (1) {
    // issue command to convert temprature to all devices
    oneWire_reset(true);
    if (!convert_DS18_temp(true)) {
      srn_print(&csr1, "\nConvert temp failed");
    }

    // for each device on the bus, read scratch and print temperature
    for (int i = 0;  i < num_devs; i++) {
      oneWire_reset(true);
      if (get_DS18_scratch(devs[i])) {
        p++;
      } else {
        print_d("\nScratch Read Failed");
        sprintf(txt, "\%d n%x",devs[i]->temperature);
        print_d(txt);
        f++;
      }
      float temp = (float)devs[i]->temperature / 16.0;
      draw_next_as_bar(&gsras,temp);
      sprintf(txt,"\n%d:%5.1f", i, temp);
      srn_print(&csr1, txt);
      sprintf(txt, "\nF=%d\nP=%d",f,p);
      srn_print(&csr2, txt);
    }
  start_blinking(false, false, true, 100);
  }
}
