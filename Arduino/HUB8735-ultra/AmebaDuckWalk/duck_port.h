/*
 * duck_port.h - the thin hardware layer.
 *
 * The policy/runtime modules are plain portable C and call only these
 * functions. duck_port_arduino.cpp implements them for the AMB82-MINI
 * Arduino core. The servo bus and the IMU go through Arduino libraries
 * instead (duck_robot.cpp: SCServo, duck_imu.cpp: 7Semi_BNO055); porting
 * to the AmebaPro2 FreeRTOS SDK means re-implementing those three files.
 */

#ifndef DUCK_PORT_H
#define DUCK_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Monotonic microsecond clock. Must not go backwards. */
uint32_t duck_port_micros(void);

/* GPIO. pin < 0 means "not fitted". */
void duck_port_gpio_input_pullup(int pin);
int  duck_port_gpio_read(int pin);       /* 0 or 1 */

/* Delay. */
void duck_port_delay_ms(uint32_t ms);

/* Console logging. */
void duck_port_log(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* DUCK_PORT_H */
