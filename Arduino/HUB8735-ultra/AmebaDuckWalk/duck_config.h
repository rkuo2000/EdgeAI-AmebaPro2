/*
 * duck_config.h - every board-specific setting for the Ameba Pro2 duck.
 *
 * This is the only file that should need editing to bring up a new robot.
 * Items marked [VERIFY] depend on the physical build and must be checked
 * on the bench before the first walk.
 */

#ifndef DUCK_CONFIG_H
#define DUCK_CONFIG_H

/* ------------------------------------------------------------------ */
/* Servo bus (Feetech STS3215, half-duplex TTL)                        */
/* ------------------------------------------------------------------ */

/* The policy was trained against servos running at 1 Mbps. Lowering this
 * requires reconfiguring every servo's baud register as well. */
#define DUCK_SERVO_BAUD             1000000UL

/* Which Arduino HardwareSerial instance drives the servo bus.
 * AMB82-MINI (ameba-arduino-pro2 variant.h): Serial1 = TX D21/PA2, RX D22/PA3;
 * Serial2 = TX D19/PD15, RX D18/PD16; Serial3 = TX D15/PE1, RX D14/PE2.
 * It must not be Serial (the USB console), and no other pin below may reuse
 * the chosen TX/RX pins. */
#define DUCK_SERVO_SERIAL           Serial2

/* SCServo per-byte receive timeout in milliseconds (SCSerial::IOTimeOut,
 * library default 100). It bounds how long a silent servo can stall one
 * 20 ms control step; millis() granularity makes 2 mean 1-3 ms. */
#define DUCK_SERVO_IO_TIMEOUT_MS    2

/* ------------------------------------------------------------------ */
/* IMU (Bosch BNO055 over I2C, 7Semi_BNO055 library)                  */
/* ------------------------------------------------------------------ */

/* Initialised like the library's examples/Basic: imu.begin() finds the
 * chip at 0x28 or 0x29 (I2C at the 100 kHz default), then setMode(NDOF)
 * and a calibration wait. This only happens once the duck stands still.
 * The axis remap is applied in software (see duck_imu.cpp). */

/* 1 to switch to the board's 32.768 kHz crystal after begin(), as the
 * 7Semi Gyroscope example does. Basic leaves it 0 (internal oscillator),
 * which always works. [VERIFY] */
#define DUCK_IMU_EXT_CRYSTAL        0

/* Matches "imu_upside_down" in the Python duck_config.json.
 * 0 = chip mounted as on the reference build. [VERIFY] */
#define DUCK_IMU_UPSIDE_DOWN        0

/* Before the BNO055 is started, every joint must report a speed below
 * DUCK_STILL_VEL_MAX (rad/s) for DUCK_STILL_HOLD_MS in a row. If the duck
 * has not settled within DUCK_STILL_TIMEOUT_MS the IMU is not started and
 * the pose is held ('u' retries). */
#define DUCK_STILL_VEL_MAX          0.10f
#define DUCK_STILL_HOLD_MS          1000
#define DUCK_STILL_TIMEOUT_MS       5000

/* Basic.ino's calibration wait (it uses 10000 ms, polling every 200 ms).
 * Here it waits for the gyro only, which calibrates while standing still.
 * A timeout is not fatal, as in Basic.ino. */
#define DUCK_IMU_CALIB_TIMEOUT_MS   10000
#define DUCK_IMU_CALIB_POLL_MS      200

/* ------------------------------------------------------------------ */
/* Foot contact switches (active low, internal pull-up)                */
/* ------------------------------------------------------------------ */

/* GPIO numbers on AMB82-MINI. Set to -1 if a switch is absent; it will then
 * read as "touching", which is what a standing duck reports.
 * D20 (PD14) and D17 (PD17) are plain GPIOs. D21/D22 are Serial1 TX/RX and
 * must not be used while the servo bus is on Serial1. */
#define DUCK_FOOT_LEFT_PIN          24
#define DUCK_FOOT_RIGHT_PIN         13

#if (DUCK_FOOT_LEFT_PIN == 21 || DUCK_FOOT_LEFT_PIN == 22 || \
     DUCK_FOOT_RIGHT_PIN == 21 || DUCK_FOOT_RIGHT_PIN == 22)
#error "D21/D22 are Serial1 TX/RX on AMB82-MINI; move the foot switch"
#endif

/* Optional emergency-stop button, active low. -1 disables it. */
#define DUCK_ESTOP_PIN              (-1)

/* ------------------------------------------------------------------ */
/* Joint calibration                                                   */
/* ------------------------------------------------------------------ */

/* Per-joint zero offset in radians, same meaning and order as
 * "joints_offsets" in the Python duck_config.json:
 *   left_hip_yaw, left_hip_roll, left_hip_pitch, left_knee, left_ankle,
 *   neck_pitch, head_pitch, head_yaw, head_roll,
 *   right_hip_yaw, right_hip_roll, right_hip_pitch, right_knee, right_ankle
 * [VERIFY] Measure on YOUR robot. Copying another build's values will
 * make the duck stand crooked or fall. Use the "calib" console command. */
#define DUCK_JOINT_OFFSETS { \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f,       \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f  \
}

/* ------------------------------------------------------------------ */
/* Servo gains (values the reference runtime uses)                     */
/* ------------------------------------------------------------------ */

#define DUCK_KP_WALK        30   /* RLWalk pid default -p 30           */
#define DUCK_KP_HEAD         8   /* RLWalk.start(): kps[5:9] = 8       */
#define DUCK_KP_SOFT         2   /* HWI.low_torque_kps                 */
#define DUCK_KD              0

/* ------------------------------------------------------------------ */
/* Gait timing                                                         */
/* ------------------------------------------------------------------ */

/* Reference-motion period: 0.54 s at 50 fps = 27 steps.
 * Read from polynomial_coefficients.pkl, not a guess. */
#define DUCK_PERIOD_STEPS           27.0f

/* "phase_frequency_factor_offset" in duck_config.json. */
#define DUCK_PHASE_FREQ_OFFSET      0.0f

/* ------------------------------------------------------------------ */
/* Safety                                                              */
/* ------------------------------------------------------------------ */

/* Cut torque if the trunk tilts past this. Body-frame gravity component
 * along +z; 9.81 upright, ~4.9 at 60 degrees, 0 on its side. */
#define DUCK_FALL_ACCEL_Z_MIN       4.0f

/* Consecutive failed servo reads before torque is cut. Each failure
 * skips one control step, exactly as the Python runtime does. */
#define DUCK_MAX_CONSEC_BUS_FAIL    10

/* Consecutive failed IMU reads before torque is cut. While the IMU is not
 * answering the policy sees the last good sample and the fall check above
 * is blind, so this must stay small (5 steps = 100 ms). */
#define DUCK_MAX_CONSEC_IMU_FAIL    5

/* Seconds to ramp from the current pose to the standing pose at boot. */
#define DUCK_STARTUP_RAMP_S         1.5f

/* Command limits, from xbox_controller.py ranges. */
#define DUCK_CMD_VX_MAX             0.15f
#define DUCK_CMD_VY_MAX             0.20f
#define DUCK_CMD_VYAW_MAX           1.00f

#endif /* DUCK_CONFIG_H */
