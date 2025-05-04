/*
AVR In-System Programming over WiFi for ESP8266
Copyright (c) Kiril Zyapkov <kiril@robotev.com>

Original version:
    ArduinoISP version 04m3
    Copyright (c) 2008-2011 Randall Bohn
    If you require a license, see
        http://www.opensource.org/licenses/bsd-license.php
*/

//
//avrdude.exe -Cavrdude.conf -v -carduino -patmega328pb -P net:esp-avrisp-test.local:328
//プログラム	>> -U flash:w:C:\avrdude\arduino-fader.ino.arduino_standard.hex
//ヒューズビット	>> -e -U lock:w:0x3F:m -U efuse:w:0xf5:m -U hfuse:w:0xde:m -U lfuse:w:0xc7:m

#ifndef _ESP_AVRISP_H
#define _ESP_AVRISP_H

#include <Arduino.h>
#include <functional>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

// uncomment if you use an n-mos to level-shift the reset line
// #define AVRISP_ACTIVE_HIGH_RESET


// programmer states
typedef enum {
  AVRISP_STATE_IDLE = 0,  // no active TCP session
  AVRISP_STATE_PENDING,   // TCP connected, pending SPI activation
  AVRISP_STATE_ACTIVE     // programmer is active and owns the SPI bus
} AVRISPState_t;

// stk500 parameters
typedef struct {
  uint8_t devicecode;
  uint8_t revision;
  uint8_t progtype;
  uint8_t parmode;
  uint8_t polling;
  uint8_t selftimed;
  uint8_t lockbytes;
  uint8_t fusebytes;
  int flashpoll;
  int eeprompoll;
  int pagesize;
  int eepromsize;
  int flashsize;
} AVRISP_parameter_t;


class ESP_AVRISP {
public:
  typedef std::function<void(void)> TCallback;
  TCallback icspBegin;

  ESP_AVRISP(uint16_t port, uint8_t reset_pin, SPIClass *icsp, bool reset_state = false, bool reset_activehigh = false);

  void begin();

  // control the state of the RESET pin of the target
  // see AVRISP_ACTIVE_HIGH_RESET
  void setReset(bool);

  // check for pending clients if IDLE, check for disconnect otherwise
  // returns the updated state
  AVRISPState_t update();

  // transition to ACTIVE if PENDING
  // serve STK500 commands from buffer if ACTIVE
  // returns the updated state
  AVRISPState_t serve();

protected:

  inline void _reject_incoming(void);  // reject any incoming tcp connections

  void avrisp(void);  // handle incoming STK500 commands

  uint8_t getch(void);  // retrieve a character from the remote end
  uint8_t spi_transaction(uint8_t, uint8_t, uint8_t, uint8_t);
  void empty_reply(void);
  void breply(uint8_t);

  void get_parameter(uint8_t);
  void set_parameters(void);
  int addr_page(int);
  void flash(uint8_t, int, uint8_t);
  void write_flash(int);
  uint8_t write_flash_pages(int length);
  uint8_t write_eeprom(int length);
  uint8_t write_eeprom_chunk(int start, int length);
  void commit(int addr);
  void program_page();
  uint8_t flash_read(uint8_t hilo, int addr);
  void flash_read_page(int length);
  void eeprom_read_page(int length);
  void read_page();
  void read_signature();

  void universal(void);

  void fill(int);          // fill the buffer with n bytes
  void start_pmode(void);  // enter program mode
  void end_pmode(void);    // exit program mode

  inline bool _resetLevel(bool reset_state) {
    return reset_state == _reset_activehigh;
  }

  WiFiServer _server;
  WiFiClient _client;
  AVRISPState_t _state;
  uint8_t _reset_pin;
  bool _reset_state;
  bool _reset_activehigh;

  SPIClass *_icsp;

  // programmer settings, set by remote end
  AVRISP_parameter_t param;
  // page buffer
  uint8_t buff[256];

  int error = 0;
  bool pmode = 0;

  // address for reading and writing, set by 'U' command
  int here;
};


#endif  // _ESP_AVRISP_H
