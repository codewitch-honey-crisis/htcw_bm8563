#include "bm8563.hpp"
#ifdef ARDUINO
using namespace arduino;
#else
#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1
#include <string.h>
using namespace esp_idf;
#endif

void bm8563::do_move(bm8563& rhs) {
    m_i2c = rhs.m_i2c;
    m_initialized = rhs.m_initialized;
}
bm8563::bm8563(bm8563&& rhs) {
    do_move(rhs);
}
bm8563& bm8563::operator=(bm8563&& rhs) {
    do_move(rhs);
    return *this;
}
bm8563::bm8563(
#ifdef ARDUINO
    TwoWire& i2c
#else
    i2c_port_t i2c
#endif
    ) : m_i2c(i2c) {
}
bool bm8563::initialized() const {
    return m_initialized;
}
bool bm8563::initialize() {
    if (!m_initialized) {
#ifdef ARDUINO
        m_i2c.begin();
        m_i2c.beginTransmission(address);
        m_i2c.write(0x00);
        m_i2c.write(0x00);
        m_i2c.write(0x00);
        if (0 == m_i2c.endTransmission()) {
            m_initialized = true;
        }
#else
        uint8_t buf[] = {0, 0, 0};
        if (ESP_OK == i2c_master_write_to_device(m_i2c, address, buf, sizeof(buf), pdMS_TO_TICKS(1000))) {
            m_initialized = true;
        }
#endif
    }
    return m_initialized;
}

bool bm8563::voltage_low() const {
    if(!initialized()) {
        return false;
    }
    uint8_t data = reg(0x02);
    return data & 0x80;  // RTCC_VLSEC_MASK
}

uint8_t bm8563::bcd_to_byte(uint8_t value) {
    uint8_t tmp = 0;
    tmp = ((uint8_t)(value & (uint8_t)0xF0) >> (uint8_t)0x4) * 10;
    return (tmp + (value & (uint8_t)0x0F));
}

uint8_t bm8563::byte_to_bcd(uint8_t value) {
    uint8_t bcdhigh = 0;

    while (value >= 10) {
        bcdhigh++;
        value -= 10;
    }

    return ((uint8_t)(bcdhigh << 4) | value);
}
void bm8563::now(tm* out_tm) const {
    if(!initialized()) {
        return;
    }
    uint8_t buf[4] = {0};
#ifdef ARDUINO
    m_i2c.beginTransmission(address);
    m_i2c.write(0x02);
    m_i2c.endTransmission();

    m_i2c.requestFrom((uint16_t)address, (size_t)3);
    if (m_i2c.available()) {
        buf[0] = m_i2c.read();
        buf[1] = m_i2c.read();
        buf[2] = m_i2c.read();
    }
#else
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x02, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(m_i2c, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | 1, ACK_CHECK_EN);
    i2c_master_read(cmd, buf, 3, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(m_i2c, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
#endif
    out_tm->tm_sec = bcd_to_byte(buf[0] & 0x7f);
    out_tm->tm_min = bcd_to_byte(buf[1] & 0x7f);
    out_tm->tm_hour = bcd_to_byte(buf[2] & 0x3f);
#ifdef ARDUINO
    m_i2c.beginTransmission(address);
    m_i2c.write(0x05);
    m_i2c.endTransmission();

    m_i2c.requestFrom((uint16_t)address, (size_t)4);
    if (m_i2c.available()) {
        buf[0] = m_i2c.read();
        buf[1] = m_i2c.read();
        buf[2] = m_i2c.read();
        buf[3] = m_i2c.read();
    }
#else
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x05, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(m_i2c, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | 1, ACK_CHECK_EN);
    i2c_master_read(cmd, buf, sizeof(buf), I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(m_i2c, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
#endif
    out_tm->tm_mday = bcd_to_byte(buf[0] & 0x3f);
    out_tm->tm_wday = bcd_to_byte(buf[1] & 0x07);
    out_tm->tm_mon = bcd_to_byte(buf[2] & 0x1f) - 1;

    if (buf[2] & 0x80) {
        out_tm->tm_year = bcd_to_byte(buf[3] & 0xff) + 00;
    } else {
        out_tm->tm_year = bcd_to_byte(buf[3] & 0xff) + 100;
    }
    out_tm->tm_isdst = -1;
}
time_t bm8563::now() const {
    if(!initialized()) {
        return (time_t)0;
    }
    tm result;
    now(&result);
    return mktime(&result);
}
void bm8563::set(const tm& new_tm) {
    if(!initialize()) {
        return;
    }
#ifdef ARDUINO
    m_i2c.beginTransmission(address);
    m_i2c.write(0x02);
    m_i2c.write(byte_to_bcd(new_tm.tm_sec));
    m_i2c.write(byte_to_bcd(new_tm.tm_min));
    m_i2c.write(byte_to_bcd(new_tm.tm_hour));
    m_i2c.endTransmission();
    m_i2c.beginTransmission(address);
    m_i2c.write(0x05);
    m_i2c.write(byte_to_bcd(new_tm.tm_mday));
    m_i2c.write(byte_to_bcd(new_tm.tm_wday));

    if (new_tm.tm_year < 100) {
        m_i2c.write(byte_to_bcd(new_tm.tm_mon + 1) | 0x80);
        m_i2c.write(byte_to_bcd((uint8_t)(new_tm.tm_year % 100)));
    } else {
        m_i2c.write(byte_to_bcd(new_tm.tm_mon + 1) | 0x00);
        m_i2c.write(byte_to_bcd((uint8_t)(new_tm.tm_year % 100)));
    }
    m_i2c.endTransmission();
#else
    uint8_t cmds1[] = {
                    0x02,
                    byte_to_bcd(new_tm.tm_sec), 
                    byte_to_bcd(new_tm.tm_min), 
                    byte_to_bcd(new_tm.tm_hour)
    };
    i2c_master_write_to_device(m_i2c, address, cmds1, sizeof(cmds1), pdMS_TO_TICKS(1000));
    uint8_t mon, yr;
    if(new_tm.tm_year<100) {
        mon = byte_to_bcd(new_tm.tm_mon + 1) | 0x80;
        yr = byte_to_bcd((uint8_t)(new_tm.tm_year % 100));
    } else {
        mon = byte_to_bcd(new_tm.tm_mon + 1) | 0x00;
        yr = byte_to_bcd((uint8_t)(new_tm.tm_year % 100));
    }
     
    uint8_t cmds2[] = {
                    0x05,
                    byte_to_bcd(new_tm.tm_mday),
                    byte_to_bcd(new_tm.tm_wday),
                    mon,
                    yr
    };
    i2c_master_write_to_device(m_i2c, address, cmds2, sizeof(cmds2), pdMS_TO_TICKS(1000));
#endif
}
void bm8563::set(time_t new_time) {
    set(*localtime(&new_time));
}
time_t bm8563::build() const {
    tm result;
    build(&result);
    return mktime(&result);
}
void bm8563::build(tm* out_tm) const {
    char s_month[5];
    int month, day, year;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf(__DATE__, "%s %d %d", s_month, &day, &year);
    month = (strstr(month_names, s_month) - month_names) / 3;

    out_tm->tm_mon = month;
    out_tm->tm_mday = day;
    out_tm->tm_year = year - 1900;
    out_tm->tm_isdst = -1;
    int d    = day   ; //Day     1-31
    int m    = month+1    ; //Month   1-12`
    int y    = year;
    // from https://stackoverflow.com/questions/6054016/c-program-to-find-day-of-week-given-date
    int weekday  = (d += m < 3 ? y-- : y - 2, 23*m/9 + d + 4 + y/4- y/100 + y/400)%7; 
    out_tm->tm_wday = weekday;
    int hr, min, sec;
    sscanf(__TIME__, "%2d:%2d:%2d", &hr, &min, &sec);
    out_tm->tm_hour = hr;
    out_tm->tm_min = min;
    out_tm->tm_sec = sec;
}
void bm8563::set_alarm(int after_seconds) {
    if(!initialize()) {
        return;
    }
    uint8_t reg_value = 0;
    reg_value = reg(0x01);

    if (after_seconds < 0) {
        reg_value &= ~(1 << 0);
        reg(0x01, reg_value);
        reg_value = 0x03;
        reg(0x0E, reg_value);
        return;
    }

    uint8_t type_value = 2;
    uint8_t div = 1;
    if (after_seconds > 255) {
        div = 60;
        type_value = 0x83;
    } else {
        type_value = 0x82;
    }

    after_seconds = (after_seconds / div) & 0xFF;
    reg(0x0F, after_seconds);
    reg(0x0E, type_value);

    reg_value |= (1 << 0);
    reg_value &= ~(1 << 7);
    reg(0x01, reg_value);
}
void bm8563::set_alarm(const tm& alarm_tm) {
    if(!initialize()) {
        return;
    }
    uint8_t out_buf[4] = {0x80, 0x80, 0x80, 0x80};

    out_buf[0] = byte_to_bcd(alarm_tm.tm_min) & 0x7f;
    out_buf[1] = byte_to_bcd(alarm_tm.tm_hour) & 0x3f;

    out_buf[2] = byte_to_bcd(alarm_tm.tm_mday) & 0x3f;

    if (alarm_tm.tm_wday >= 0) {
        out_buf[3] = byte_to_bcd(alarm_tm.tm_wday) & 0x07;
    }

    uint8_t reg_value = reg(0x01);

    reg_value |= (1 << 1);

    for (int i = 0; i < 4; i++) {
        reg(0x09 + i, out_buf[i]);
    }
    reg(0x01, reg_value);
}
void bm8563::dismiss_alarm() {
    if(!initialize()) {
        return;
    }
    reg(0x01, reg(0x01) & 0xf3);
}
void bm8563::clear_alarm() {
    dismiss_alarm();
    reg(0x01, reg(0x01) & 0xfC);
}
void bm8563::reg(uint8_t r, uint8_t value) {
#ifdef ARDUINO
    m_i2c.beginTransmission(bm8563::address);
    m_i2c.write(r);
    m_i2c.write(value);
    m_i2c.endTransmission();
#else
    uint8_t data[] = {r,value};
    i2c_master_write_to_device(m_i2c,address,data,sizeof(data),pdMS_TO_TICKS(1000));
#endif
}

uint8_t bm8563::reg(uint8_t r) const {
#ifdef ARDUINO
    m_i2c.beginTransmission(bm8563::address);
    m_i2c.write(r);
    m_i2c.endTransmission();
    m_i2c.requestFrom((uint16_t)bm8563::address, (size_t)1);
    return m_i2c.read();
#else
    uint8_t result;
    if(ESP_OK==i2c_master_write_read_device(m_i2c,address,&r,1,&result,1,pdMS_TO_TICKS(1000))) {
        return result;
    }
    return 0;
#endif
}
