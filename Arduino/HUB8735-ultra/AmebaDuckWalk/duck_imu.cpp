/*
 * duck_imu.cpp - duck_imu.h on top of the 7Semi_BNO055 library, following
 * examples/Basic/Basic.ino:
 *
 *   imu.begin()                 auto-detect 0x28/0x29, chip id check
 *   imu.setMode(Mode::NDOF)     fusion mode
 *   wait for calibration        printCalib() / calibBreakdown()
 *   imu.readAccel(), imu.readGyro()
 *
 * Differences from Basic.ino, all needed on the robot:
 *   - Basic waits for imu.waitCalibrated(), i.e. SYS, G, A and M all at 3.
 *     A and M only reach 3 after the sensor is turned through several
 *     orientations / waved in a figure 8, which a standing duck never does,
 *     so that wait would always run into its timeout. Only the gyro level is
 *     waited for here; it reaches 3 after a few seconds of standing still.
 *   - The wait polls an abort hook so 'k' and the e-stop still work.
 *   - Raw values are scaled and remapped to the reference robot's frame.
 *
 * Output scaling is the same as adafruit_bno055 (default UNIT_SEL):
 *   gyro  : raw / 16 dps  -> raw * 0.001090830782496456 rad/s
 *   accel : raw / 100     -> m/s^2 (includes gravity)
 *
 * The reference robot remaps the axes in the chip (AXIS_MAP_CONFIG 0x21 =
 * X<-Y, Y<-X, Z<-Z; AXIS_MAP_SIGN 0x04 = negate X, or 0x07 = negate all
 * when upside down). The remap applies identically to the accel and gyro
 * registers, so it is done here in software instead:
 *   upright     : (x, y, z) -> (-y,  x,  z)
 *   upside down : (x, y, z) -> (-y, -x, -z)
 */

#include <Arduino.h>
#include <Wire.h>
#include <7Semi_BNO055.h>

extern "C" {
#include "duck_imu.h"
#include "duck_port.h"
}

#define GYRO_SCALE          0.001090830782496456f
#define ACCEL_SCALE         0.01f

static BNO055_7Semi imu;
static int s_upside_down;

extern "C" void duck_imu_print_calib(void)
{
    uint8_t sys, gyr, acc, mag;
    imu.calibBreakdown(sys, gyr, acc, mag);
    duck_port_log("[imu] Calib SYS:%u G:%u A:%u M:%u", sys, gyr, acc, mag);
}

extern "C" int duck_imu_init(int upside_down, int ext_crystal)
{
    s_upside_down = upside_down;

    /* Initialize (Wire, auto-detect address, internal crystal) */
    if (!imu.begin()) {
        duck_port_log("[imu] ERROR: BNO055 not found (no ACK at 0x28/0x29 or "
                      "chip id != 0xA0) - check SDA/SCL wiring and power");
        return -1;
    }

    imu.setMode(Mode::NDOF);

    if (ext_crystal && !imu.enableExternalCrystal(true)) {
        /* Not fatal: the internal oscillator works, only less accurately. */
        duck_port_log("[imu] external crystal enable failed, using internal");
    }
    duck_port_log("[imu] BNO055 found, chip id 0x%02X, mode 0x%02X, %d C",
                  imu.readReg(REG_CHIP_ID), imu.readReg(REG_OPR_MODE),
                  (int)imu.temperatureC());
    return 0;
}

extern "C" int duck_imu_wait_calibrated(uint32_t timeout_ms, uint32_t poll_ms,
                                        int (*abort_hook)(void))
{
    uint32_t start = millis();
    uint32_t last_print = start;
    uint8_t sys, gyr, acc, mag;
    int rc = 0;

    duck_port_log("[imu] Calibrating - keep the duck still");
    for (;;) {
        imu.calibBreakdown(sys, gyr, acc, mag);
        if (gyr == 3) {
            rc = 1;
            break;
        }
        if (millis() - start >= timeout_ms) {
            break;
        }
        if (abort_hook && abort_hook()) {
            rc = -2;
            break;
        }
        if (millis() - last_print >= 1000) {
            last_print = millis();
            duck_imu_print_calib();
        }
        delay(poll_ms);
    }
    duck_port_log(rc == 1 ? "[imu] Calibrating - done" :
                  rc == 0 ? "[imu] Calibrating - timeout" :
                            "[imu] Calibrating - aborted");
    duck_imu_print_calib();
    return rc;
}

extern "C" int duck_imu_read(float gyro[3], float accel[3])
{
    int16_t ax, ay, az, gx, gy, gz;
    const float s = s_upside_down ? -1.0f : 1.0f;

    if (!imu.readAccel(ax, ay, az) || !imu.readGyro(gx, gy, gz)) {
        return -1;
    }
    accel[0] = -ay * ACCEL_SCALE;
    accel[1] = s * ax * ACCEL_SCALE;
    accel[2] = s * az * ACCEL_SCALE;
    gyro[0]  = -gy * GYRO_SCALE;
    gyro[1]  = s * gx * GYRO_SCALE;
    gyro[2]  = s * gz * GYRO_SCALE;
    return 0;
}
