#include <bm8563.hpp>
#include <time.h>
bm8563::bm8563(TwoWire& i2c) : m_i2c(i2c) {
}

void bm8563::initialize(void) {
    m_i2c.beginTransmission(address);
    m_i2c.write(0x00);
    m_i2c.write(0x00);
    m_i2c.write(0x00);
    m_i2c.endTransmission();
}

bool bm8563::voltage_low() const {
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
    uint8_t buf[4] = {0};
    m_i2c.beginTransmission(address);
    m_i2c.write(0x02);
    m_i2c.endTransmission();
    m_i2c.requestFrom((uint16_t)address, (size_t)3);
    if (m_i2c.available()) {
        buf[0] = m_i2c.read();
        buf[1] = m_i2c.read();
        buf[2] = m_i2c.read();
    }
    out_tm->tm_sec = bcd_to_byte(buf[0] & 0x7f);
    out_tm->tm_min = bcd_to_byte(buf[1] & 0x7f);
    out_tm->tm_hour = bcd_to_byte(buf[2] & 0x3f);
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
    tm result;
    now(&result);
    return mktime(&result);
}
void bm8563::set(const tm& new_tm) {
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
        m_i2c.write(byte_to_bcd(new_tm.tm_mon+1) | 0x80);
        m_i2c.write(byte_to_bcd((uint8_t)(new_tm.tm_year % 100)));
    } else {
        m_i2c.write(byte_to_bcd(new_tm.tm_mon+1) | 0x00);
        m_i2c.write(byte_to_bcd((uint8_t)(new_tm.tm_year % 100)));
    }
    m_i2c.endTransmission();
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
    month = (strstr(month_names, s_month)-month_names)/3;

    out_tm->tm_mon = month;
    out_tm->tm_mday = day;
    out_tm->tm_year = year - 1900;
    out_tm->tm_isdst = -1;
    
    int hr,min,sec;
    sscanf(__TIME__,"%2d:%2d:%2d",&hr,&min,&sec);
    out_tm->tm_hour = hr;
    out_tm->tm_min = min;
    out_tm->tm_sec = sec;
}
void bm8563::set_alarm(int after_seconds) {
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
    reg(0x01, reg(0x01) & 0xf3);    
}
void bm8563::clear_alarm() {
    dismiss_alarm();
    reg(0x01, reg(0x01) & 0xfC);

}
void bm8563::reg(uint8_t address, uint8_t value) {
    m_i2c.beginTransmission(bm8563::address);
    m_i2c.write(address);
    m_i2c.write(value);
    m_i2c.endTransmission();
}

uint8_t bm8563::reg(uint8_t address) const {
    m_i2c.beginTransmission(bm8563::address);
    m_i2c.write(address);
    m_i2c.endTransmission();
    m_i2c.requestFrom((uint16_t)bm8563::address, (size_t)1);
    return m_i2c.read();
}

