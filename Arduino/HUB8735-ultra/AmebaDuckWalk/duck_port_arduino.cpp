/*
 * duck_port_arduino.cpp - duck_port.h for the AMB82-MINI Arduino core.
 *
 * Together with duck_robot.cpp and duck_imu.cpp, the only file that
 * touches Arduino APIs.
 */

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

extern "C" {
#include "duck_port.h"
}

extern "C" uint32_t duck_port_micros(void)
{
    return (uint32_t)micros();
}

extern "C" void duck_port_delay_ms(uint32_t ms)
{
    delay(ms);
}

/* ---------------- GPIO ---------------- */

extern "C" void duck_port_gpio_input_pullup(int pin)
{
    if (pin >= 0) {
        pinMode(pin, INPUT_PULLUP);
    }
}

extern "C" int duck_port_gpio_read(int pin)
{
    if (pin < 0) {
        return 1;
    }
    return digitalRead(pin) ? 1 : 0;
}

/* ---------------- console ---------------- */

extern "C" void duck_port_log(const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Serial.println(buf);
}
