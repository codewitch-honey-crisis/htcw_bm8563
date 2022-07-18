#pragma once
#include <Arduino.h>
#include <Wire.h>
// used the BM8563 library by Tanaka Masayuki
// as a reference implementation
// https://www.arduino.cc/reference/en/libraries/i2c-bm8563-rtc/

class bm8563 {
  public:
    constexpr static const uint8_t address = 0x51;
    bm8563(TwoWire &i2c = Wire);

    void initialize(void);
    bool voltage_low() const;
    time_t now() const;
    void now(tm* out_tm) const;
    time_t build() const;
    void build(tm* out_tm) const;
    void set(time_t new_time);
    void set(const tm& new_tm);
    void set_alarm(int after_seconds);
    void set_alarm(const tm& alarm_tm);
    void dismiss_alarm();
    void clear_alarm();
private:
    void reg(uint8_t address, uint8_t value);
    uint8_t reg(uint8_t address) const;
    static uint8_t bcd_to_byte(uint8_t value);
    static uint8_t byte_to_bcd(uint8_t value);
    TwoWire& m_i2c;
};
