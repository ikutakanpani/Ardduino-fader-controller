#ifndef moving_Average_hpp
#define moving_Average_hpp

class Average {
public:
  Average(){
    _average_total = 0;
  }

  Average(uint8_t Average_total) {
    _average_total = Average_total;
    if (_average_total > 50) _average_total = 50;
  }

  Average(uint8_t Average_total, boolean hold) {
    _hold = hold;
    _hold = _hold_set;
    _average_total = Average_total;
    if (_average_total > 50) _average_total = 50;
  }

  void reset_total(){
    _average_sum = 0;
    _average_cnt = 0;
    _hold = _hold_set;
  }

  void set_total(uint8_t Average_total) {
    if (Average_total > 50) return;

    _average_total = Average_total;
    _average_sum = 0;
    _average_cnt = 0;
    _hold = _hold_set;
  }

  void set_hold(boolean hold) {
    _hold_set = hold;
    _hold = _hold_set;
  }

  double input(double i) {
    if (_average_total == 0) {
      _average_val = i;
      return _average_val;
    }

    average_run(i);

    return _average_val;
  }

  double read_average() {
    return _average_val;
  }

private:
  double _average_buf[50];
  boolean _hold_set = true;
  boolean _hold = _hold_set;
  uint8_t _average_cnt = 0;
  uint8_t _average_total = 0;
  double _average_val = 0;
  double _average_sum = 0;

  void average_run(double i) {
    _average_sum -= _average_buf[_average_cnt];

    _average_buf[_average_cnt] = i;

    _average_sum += _average_buf[_average_cnt];

    _average_cnt++;
    if (_average_cnt == _average_total) {
      _average_cnt = 0;
      _hold = false;
    }

    if(_average_sum != 0){
      if (!_hold) _average_val = _average_sum / _average_total;
      else _average_val = _average_sum / _average_cnt;
    }

  }
};
#endif
