#ifndef Arduino_fader_hpp
#define Arduino_fader_hpp

#include <Arduino.h>
#include <functional>

class ArduinoFader {
public:
  typedef std::function<void()> TCallback;
  typedef std::function<void(uint8_t, boolean)> TCallback_cs;

  ArduinoFader() {
  }
  ArduinoFader(SPIClass *spi, uint8_t spi_cs) {
    _spi = spi;
    _spi_cs = spi_cs;
  }
  ArduinoFader(SPIClass *spi, uint8_t spi_cs, boolean callbackEnable) {
    _spi = spi;
    _spi_cs = spi_cs;
    _callbackEnable = callbackEnable;
  }

  void initialize(SPIClass *spi, uint8_t spi_cs) {
    _spi = spi;
    _spi_cs = spi_cs;
  }
  void initialize(SPIClass *spi, uint8_t spi_cs, boolean callbackEnable) {
    _spi = spi;
    _spi_cs = spi_cs;
    _callbackEnable = callbackEnable;
  }

  boolean enabled() {
    _sys_status = true;
    statusSend();
    delay(100);
    statusRead();
    delay(100);
    return _R_sys_status;
  }
  boolean disabled() {
    _sys_status = false;
    statusSend();
    delay(100);
    statusRead();
    delay(100);
    _R_sys_status = false;
    return _R_sys_status;
  }

  //0:kp 1:ki 2:kd 3:cycle
  int commandWrite(uint8_t reg, int val) {
    uint16_t send_val;

    if (!_R_sys_status) return 0;

    switch (reg) {
      case 3:
        send_val = val;
        if (send_val > 19) send_val = 19;
        break;
      default:
        return 0;
    }

    return command_send(reg + 12, send_val);
  }
  float commandWrite(uint8_t reg, float val) {
    uint16_t send_val;

    if (!_R_sys_status) return 0;

    switch (reg) {
      case 0:
      case 1:
      case 2:
        send_val = val * 100;
        if (send_val > 4095) send_val = 4095;
        break;
      default:
        return 0;
    }

    return PIDcommand_send(reg + 12, send_val);
  }

  //フェーダーをセンターモードにする
  //手動でフェーダーを動かしても離すと中心に戻る
  //mode = true: 有効  false: 解除
  void set_center_mode(boolean mode) {
    if (!_R_sys_status) return;
    _centerMode_status = mode;
  }

  //フェーダーをTargetの位置で固定を強制する
  //手動でフェーダーを動かしても戻ろうとする
  //lock = true: 有効  false: 解除
  void set_manual_lock(boolean lock) {
    if (!_R_sys_status) return;
    _manual_lock = lock;
  }

  //フェーダーの移動を指示
  // target = : 0 ~ 1023
  void set_target(uint16_t target) {
    if (!_R_sys_status) return;
    _target = target;
  }

  //フェーダーの位置を返す
  // 0 ~ 1023
  uint16_t get_faderVal() {
    if (!_R_sys_status) return 0;
    return _fader_val;
  }

  //フェーダーのロック状態を取得
  // True : フェーダーの位置がTargetで指定した位置で制止している状態
  // False: ↑の対義
  boolean get_lockStatus() {
    if (!_R_sys_status) return false;
    return _lock_status;
  }

  //フェーダーのタッチセンサを取得
  // True : フェーダーに触れている
  // False: ↑の対義
  boolean get_touchStatus() {
    if (!_R_sys_status) return false;
    return _touch_status;
  }

  boolean get_manual_lockStatus() {
    if (!_R_sys_status) return false;
    return _manual_lock;
  }

  //データ更新（送受信）
  boolean refresh() {
    uint8_t send_buf[3];
    uint8_t read_buf[3];
    uint8_t reg = 0;

    if (!_R_sys_status) return false;

    send_buf[0] = 0b01000000;
    send_buf[0] |= (0b1111 & reg) << 2;
    send_buf[0] |= _manual_lock ? 0b00000001 : 0;
    send_buf[0] |= _centerMode_status ? 0b00000010 : 0;
    send_buf[1] = 0b10000000;
    send_buf[1] |= 0b00001111 & _target;
    send_buf[1] |= _sys_status ? 0b00010000 : 0;
    send_buf[1] |= false ? 0b00100000 : 0;
    send_buf[2] = 0b11000000;
    send_buf[2] |= 0b00111111 & (_target >> 4);

    boolean status = dataRW(send_buf, read_buf);

    if (!status) {
      disabled();
      return status;
    }

    _fader_val = 0b00111100 & read_buf[2];
    _fader_val = (_fader_val << 4) | (0b00111111 & read_buf[0]);
    read_status_set(read_buf[1]);

    if (_centerResult_status) {
      if (centerResult) centerResult();
    }

    return status;
  }

  TCallback_cs callCS;
  TCallback centerResult;
private:
  SPIClass *_spi;
  uint8_t _spi_cs;
  boolean _callbackEnable;

  boolean _sys_status;
  boolean _R_sys_status;

  boolean _touch_status;
  boolean _lock_status;
  boolean _manual_lock;
  boolean _R_manual_lock;
  boolean _centerMode_status;
  boolean _R_centerMode_status;
  boolean _centerResult_status;
  uint16_t _fader_val;
  uint16_t _target;

  void read_status_set(uint8_t data) {
    _touch_status = data & 1;
    _lock_status = (data >> 1) & 1;
    _R_manual_lock = (data >> 2) & 1;
    _centerResult_status = (data >> 3) & 1;
    _R_centerMode_status = (data >> 4) & 1;
    _R_sys_status = (data >> 5) & 1;
  }

  void statusRead(){
    uint8_t read_buf[3];

    if(data_request_send(1, read_buf)){
      read_status_set(read_buf[1]);
    }
  }

  boolean statusSend(){
    uint8_t send_buf[3];
    uint8_t read_buf[3];
    uint8_t reg = 1;

    send_buf[0] = 0b01000000;
    send_buf[0] |= (0b1111 & reg) << 2;
    send_buf[0] |= _manual_lock ? 0b00000001 : 0;
    send_buf[0] |= _centerMode_status ? 0b00000010 : 0;
    send_buf[1] = 0b10000000;
    send_buf[1] |= _sys_status ? 0b00010000 : 0;
    send_buf[1] |= false ? 0b00100000 : 0;
    send_buf[2] = 0b11000000;

    boolean status = dataRW(send_buf, read_buf);

    if (status) {
      read_status_set(read_buf[1]);
    }

    return status;
  }

  //command組み立て
  boolean data_request_send(uint8_t reg, uint8_t *read_buf) {
    uint8_t send_buf[3];

    send_buf[0] = 0b00000011;
    send_buf[0] |= (0b1111 & reg) << 2;
    send_buf[1] = 0b10000000;
    send_buf[2] = 0b11000000;

    boolean status = dataRW(send_buf, read_buf);

    return status;
  }

  //command組み立て
  int command_send(uint8_t reg, uint16_t send_val) {
    uint8_t send_buf[3];
    uint8_t read_buf[3];

    send_buf[0] = 0b01000000;
    send_buf[0] |= (0b1111 & reg) << 2;
    send_buf[1] = 0b10000000;
    send_buf[1] |= 0b00111111 & send_val;
    send_buf[2] = 0b11000000;
    send_buf[2] |= 0b00111111 & (send_val >> 6);

    boolean status = dataRW(send_buf, read_buf);
    delay(100);

    if (status) {
      uint8_t tmp = 0b00111111 & read_buf[0];
      return tmp;
    }

    return -1;
  }

  //PIDcommand組み立て
  float PIDcommand_send(uint8_t reg, uint16_t send_val) {
    uint8_t send_buf[3];
    uint8_t read_buf[3];

    send_buf[0] = 0b01000000;
    send_buf[0] |= (0b1111 & reg) << 2;
    send_buf[1] = 0b10000000;
    send_buf[1] |= 0b00111111 & send_val;
    send_buf[2] = 0b11000000;
    send_buf[2] |= 0b00111111 & (send_val >> 6);

    boolean status = dataRW(send_buf, read_buf);
    delay(100);

    if (status) {
      float tmp = (uint16_t(0b00111111 & read_buf[1]) << 6) | (0b00111111 & read_buf[0]);
      return tmp / 100;
    }

    return -1;
  }

  //データ送受信
  boolean dataRW(uint8_t *send_buf, uint8_t *read_buf) {

    if (_callbackEnable) {
      if (callCS) callCS(_spi_cs, false);
    } else {
      digitalWrite(_spi_cs, LOW);
    }
    delayMicroseconds(100);

    uint8_t Parity = 0b00111100 | parity(send_buf);

    _spi->transfer(Parity);
    delayMicroseconds(400);
    _spi->transfer(send_buf[0]);
    delayMicroseconds(500);
    read_buf[0] = _spi->transfer(send_buf[1]);
    delayMicroseconds(500);
    read_buf[1] = _spi->transfer(send_buf[2]);
    delayMicroseconds(500);
    read_buf[2] = _spi->transfer(0);

    if (_callbackEnable) {
      if (callCS) callCS(_spi_cs, true);
    } else {
      digitalWrite(_spi_cs, HIGH);
    }
    delayMicroseconds(100);

    Parity = (read_buf[2] & 0b01);
    read_buf[2] = read_buf[2] & 0b11111110;
    if (Parity == parity(read_buf)) {
      if ((read_buf[2] & 0b00000010) == 0b10){
        return 1;
      }
    }
    return 0;
  }

  uint8_t parity(uint8_t *buf) {
    uint32_t val = buf[0];
    val = val << 8;
    val |= buf[1];
    val = val << 8;
    val |= buf[2];

    val ^= val >> 16;
    val ^= val >> 8;
    val ^= val >> 4;
    val ^= val >> 2;
    val ^= val >> 1;

    return val & 0x00000001;
  }
};
#endif