#ifndef SC8721_h
#define SC8721_h
#include "Arduino.h"
#include "Wire.h"

class SC8721 {
public:
  SC8721() {
    for (uint8_t i = 0; i < 10; i++) read_reg[i] = 0;
  }

  boolean vout_setting(float v);
  boolean cso_setting(float a);
  boolean status_all_read();
  void command_send(uint8_t reg, uint8_t data);

  uint8_t CSO_SET;
  uint8_t SLOPE_COMP;
  float VOUT_SET;
  boolean FB_SEL;
  boolean FB_ON;
  boolean FB_DIR;
  boolean DIS_DCDC;
  boolean REG_LOAD;
  boolean EN_PWM;
  boolean EXT_DT;
  boolean EN_VINREG;
  uint8_t FREQ_SET;
  boolean VOUT_SHORT;
  boolean VOUT_VIN_H;
  boolean THD;
  boolean OCP;
  boolean VINOVP;
  boolean VINREG_FLAG;
  boolean IBUS_FLAG;
  
private:  
  uint8_t _iic_addr = 0x62;
  uint8_t read_reg[10];
  
};

#endif