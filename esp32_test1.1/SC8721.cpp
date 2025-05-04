#include "SC8721.h"

boolean SC8721::vout_setting(float v) {
  int write_v;

  if (!SC8721::status_all_read()) return false;

  if (!EN_VINREG) {
    SC8721::command_send(0x06, read_reg[5] & 0b01111111);
  }

  if (v > 5) {
    write_v = (v - 5) / 0.02;
    SC8721::command_send(0x03, (write_v >> 2) & 0xff);
    SC8721::command_send(0x04, (write_v & 0b00000011) | 0b00011000);
  } else if (v < 5) {
    write_v = (5 - v) / 0.02;
    SC8721::command_send(0x03, (write_v >> 2) & 0xff);
    SC8721::command_send(0x04, (write_v & 0b00000011) | 0b00011100);
  } else {
    SC8721::command_send(0x03, 0);
    SC8721::command_send(0x04, 0b00011000);
  }

  SC8721::command_send(0x05, read_reg[4] | 0b00000010);
  SC8721::command_send(0x06, read_reg[5] & 0b11101111);

  if (!SC8721::status_all_read()) return false;
  return true;
}

boolean SC8721::cso_setting(float a) {
  uint8_t write_a;

  if (!SC8721::status_all_read()) return false;

  write_a = (a * 42.5) - 1;
  SC8721::command_send(0x01, write_a);
  
  if (!SC8721::status_all_read()) return false;
  return true;
}

boolean SC8721::status_all_read() {
  uint16_t VOUT_read;
  Wire.beginTransmission(_iic_addr);
  Wire.write(0x01);
  Wire.endTransmission(false);
  uint8_t bytesReceived = Wire.requestFrom(_iic_addr, 10);

  if ((bool)bytesReceived) {  //If received more than zero bytes
    for (uint8_t i = 0; i < 10; i++) read_reg[i] = Wire.read();
    CSO_SET = read_reg[0];                                //01H  uint8_t
    SLOPE_COMP = read_reg[1] & 0b00000011;                //02H  uint8_t
    VOUT_read = (uint16_t)read_reg[2] << 2;                               //03H
    VOUT_read |= read_reg[3] & 0b00000011;  //04H
    FB_SEL = (bool)(read_reg[3] & 0b00010000);            //04H
    FB_ON = (bool)(read_reg[3] & 0b00001000);             //04H
    FB_DIR = (bool)(read_reg[3] & 0b00000100);            //04H
    if (FB_DIR)VOUT_SET = 5 - ((float)VOUT_read * 0.02);
    else VOUT_SET = ((float)VOUT_read * 0.02) + 5;
    DIS_DCDC = (bool)(read_reg[4] & 0b00000100);     //05H
    REG_LOAD = (bool)(read_reg[4] & 0b00000010);     //05H
    EN_PWM = (bool)(read_reg[5] & 0b10000000);       //06H
    EXT_DT = (bool)(read_reg[5] & 0b01000000);       //06H
    EN_VINREG = (bool)(read_reg[5] & 0b00010000);    //06H
    FREQ_SET = read_reg[7] & 0b00000011;             //08H  uint8_t
    VOUT_SHORT = (bool)(read_reg[8] & 0b10000000);   //09H
    VOUT_VIN_H = (bool)(read_reg[8] & 0b01000000);  //09H
    THD = (bool)(read_reg[8] & 0b00001000);          //09H
    OCP = (bool)(read_reg[8] & 0b00000001);          //09H
    VINOVP = (bool)(read_reg[9] & 0b10000000);       //10H
    VINREG_FLAG = (bool)(read_reg[9] & 0b00000100);  //10H
    IBUS_FLAG = (bool)(read_reg[9] & 0b00000010);    //10H
    return true;
  }
  return false;
}

void SC8721::command_send(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(_iic_addr);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission(true);
}

/*
void data_print() {
  Serial.print("CSO_SET :");
  Serial.println(DDC.CSO_SET);
  Serial.print("SLOPE_COMP :");
  Serial.println(DDC.SLOPE_COMP);
  Serial.print("VOUT_SET :");
  Serial.println(DDC.VOUT_SET);
  Serial.print("FB_SEL :");
  Serial.println(DDC.FB_SEL);
  Serial.print("FB_ON :");
  Serial.println(DDC.FB_ON);
  Serial.print("FB_DIR :");
  Serial.println(DDC.FB_DIR);
  Serial.print("DIS_DCDC :");
  Serial.println(DDC.DIS_DCDC);
  Serial.print("REG_LOAD :");
  Serial.println(DDC.REG_LOAD);
  Serial.print("EN_PWM :");
  Serial.println(DDC.EN_PWM);
  Serial.print("EXT_DT :");
  Serial.println(DDC.EXT_DT);
  Serial.print("EN_VINREG :");
  Serial.println(DDC.EN_VINREG);
  Serial.print("FREQ_SET :");
  Serial.println(DDC.FREQ_SET);
  Serial.print("VOUT_SHORT :");
  Serial.println(DDC.VOUT_SHORT);
  Serial.print("VOUT_VIN_H :");
  Serial.println(DDC.VOUT_VIN_H);
  Serial.print("THD :");
  Serial.println(DDC.THD);
  Serial.print("OCP :");
  Serial.println(DDC.OCP);
  Serial.print("VINOVP :");
  Serial.println(DDC.VINOVP);
  Serial.print("VINREG_FLAG :");
  Serial.println(DDC.VINREG_FLAG);
  Serial.print("IBUS_FLAG :");
  Serial.println(DDC.IBUS_FLAG);
  Serial.print("EN_VINREG :");
  Serial.println(DDC.EN_VINREG);
  Serial.println();
}
*/
