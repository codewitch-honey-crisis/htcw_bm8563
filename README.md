# bm8563

This library allows you to use the BM8563 real time clock from Arduino or the ESP-IDF

```
[env:node32s]
platform = espressif32
board = node32s
framework = arduino
lib_deps = 
	codewitch-honey-crisis/htcw_bm8563
lib_ldf_mode = deep
```

```cpp
#include <Arduino.h>
#include <bm8563.h>
using namespace arduino;
bm8563 rtc;

void setup() {
    rtc.initialize();
    // set the clock to the build time
    rtc.set(rtc.build());
    // get the time
    time_t t = rtc.now();
}
void loop() {
}
```
