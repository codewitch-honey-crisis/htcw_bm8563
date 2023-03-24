#pragma once
#if __has_include(<Arduino.h>)
#include <Arduino.h>
#include <Wire.h>
namespace arduino {
#else
#ifndef ESP_PLATFORM
#error "This library requires the Arduino framework or an ESP32."
#endif
#include <driver/i2c.h>
#include <time.h>
namespace esp_idf {
#endif

// used the BM8563 library by Tanaka Masayuki
// as a reference implementation
// https://www.arduino.cc/reference/en/libraries/i2c-bm8563-rtc/

class bm8563 {
#ifdef ARDUINO
    TwoWire& m_i2c;
#else
    i2c_port_t m_i2c;
#endif
  bool m_initialized;
  public:
    constexpr static const uint8_t address = 0x51;
    bm8563(
#ifdef ARDUINO
      TwoWire &i2c = Wire
#else
      i2c_port_t i2c = I2C_NUM_0
#endif
      );
    bm8563(bm8563&& rhs);
    bm8563& operator=(bm8563&& rhs);
    bool initialized() const;
    bool initialize();
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
    bm8563(const bm8563& rhs)=delete;
    bm8563& operator=(const bm8563& rhs)=delete;
    void do_move(bm8563& rhs);
    void reg(uint8_t address, uint8_t value);
    uint8_t reg(uint8_t address) const;
    static uint8_t bcd_to_byte(uint8_t value);
    static uint8_t byte_to_bcd(uint8_t value);
  
};
}