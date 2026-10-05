/*
 * duck_imu.h - Bosch BNO055 IMU through the 7Semi_BNO055 library, set up
 * as in the library's examples/Basic/Basic.ino: begin(), setMode(NDOF),
 * wait for calibration, then readAccel()/readGyro().
 *
 * Outputs use the same axes and units as adafruit_bno055 in
 * mini_bdx_runtime/raw_imu.py on the reference robot.
 */
#ifndef DUCK_IMU_H
#define DUCK_IMU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* imu.begin() + imu.setMode(Mode::NDOF), as in Basic.ino setup().
 * The library probes 0x28 then 0x29 by itself. upside_down matches
 * "imu_upside_down" in duck_config.json. Returns 0 on success. */
int duck_imu_init(int upside_down, int ext_crystal);

/* Basic.ino's "Calibrating" step. The duck must be standing still: the
 * BNO055 only calibrates its gyro while it does not move. Polls every
 * poll_ms until the gyro reports calibration level 3 or timeout_ms passes,
 * printing the calibration status as it goes.
 * abort_hook (may be NULL) is polled too; non-zero cancels the wait.
 * Returns 1 = gyro calibrated, 0 = timeout (not fatal), -2 = aborted. */
int duck_imu_wait_calibrated(uint32_t timeout_ms, uint32_t poll_ms,
                             int (*abort_hook)(void));

/* Basic.ino printCalib(): "Calib SYS:x G:x A:x M:x". */
void duck_imu_print_calib(void);

/* gyro in rad/s, accel in m/s^2 (gravity included). Returns 0 on success. */
int duck_imu_read(float gyro[3], float accel[3]);

#ifdef __cplusplus
}
#endif

#endif /* DUCK_IMU_H */
