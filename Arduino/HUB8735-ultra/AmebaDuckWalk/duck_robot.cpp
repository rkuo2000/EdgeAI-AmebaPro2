/*
 * duck_robot.cpp - hardware interface, the counterpart of
 * mini_bdx_runtime/rustypot_position_hwi.py (class HWI).
 *
 * The STS3215 servo bus is driven directly with the SCServo library
 * (SMS_STS), set up and used as in its examples/AmebaSTS sketches.
 *
 * Unit conversions are copied from the rustypot crate the reference runtime
 * is built on (branch 66-support-for-feetech-motors-fts3215,
 * src/device/feetech_sts3215.rs, mod conv):
 *
 *   dxl_pos_to_radians(p)  = 2*pi*p/4096 - pi
 *   radians_to_dxl_pos(r)  = (i16)(4096*(pi + r)/(2*pi))     (truncating)
 *   dxl_to_speed(v)        = sign-magnitude bit 15, then 2*pi*v/4095
 *                            (SCServo's ReadSpeed() already undoes the
 *                            sign-magnitude, so only the scale is here)
 *
 * HWI then subtracts/adds the per-joint offset and rounds reads to 1e-3
 * (np.around(..., 3)). Both are reproduced here.
 */

#include <Arduino.h>
#include <SCServo.h>

#include "duck_robot.h"
#include "duck_config.h"
#include "duck_port.h"

#include <math.h>
#include <string.h>

#define PI_D 3.14159265358979323846

static const float k_offsets[DUCK_ACT_DIM] = DUCK_JOINT_OFFSETS;

/* STS3215 PID gain registers (SMS_STS.h has no names for these). */
#define STS_P_COEF 21
#define STS_D_COEF 22

SMS_STS sms_sts;

static int (*s_abort_hook)(void) = 0;

/* ------------------------------------------------------------------ */
/* Pure conversions (unit-tested on the host)                          */
/* ------------------------------------------------------------------ */

double duck_raw_to_rad(int16_t raw)
{
    return (2.0 * PI_D * (double)raw / 4096.0) - PI_D;
}

int16_t duck_rad_to_raw(double rad)
{
    double v = 4096.0 * (PI_D + rad) / (2.0 * PI_D);
    /* Rust `as i16` saturates; C casts of out-of-range floats are UB,
     * so clamp first. Position mode only accepts 0..4095 anyway. */
    if (v < 0.0) {
        v = 0.0;
    }
    if (v > 4095.0) {
        v = 4095.0;
    }
    return (int16_t)v;           /* truncation toward zero, as in Rust */
}

double duck_speed_to_rads(int16_t steps)
{
    return (2.0 * PI_D * (double)steps) / (4096.0 - 1.0);
}

/* np.around(x, 3): round half to even at the third decimal. */
double duck_round3(double x)
{
    return nearbyint(x * 1000.0) / 1000.0;
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static void ids_all(u8 ids[DUCK_ACT_DIM])
{
    int i;
    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        ids[i] = duck_servo_id[i];
    }
}

/* One byte per servo at reg, as one broadcast (no reply). */
static int write_u8_all(duck_robot_t *r, uint8_t reg, const uint8_t v[DUCK_ACT_DIM])
{
    u8 ids[DUCK_ACT_DIM];
    (void)r;
    ids_all(ids);
    sms_sts.syncWrite(ids, DUCK_ACT_DIM, reg, (u8 *)v, 1);
    return 0;
}

static int write_u8_same(duck_robot_t *r, uint8_t reg, uint8_t value)
{
    uint8_t v[DUCK_ACT_DIM];
    memset(v, value, sizeof(v));
    return write_u8_all(r, reg, v);
}

/* AmebaSTS_SyncWritePos, but with speed and acc left at 0 (= no limit)
 * instead of 3400/50: the policy expects each servo to chase its goal at
 * full speed, as with the reference runtime's goal-only write. */
static int write_goal_raw(duck_robot_t *r, const int16_t raw[DUCK_ACT_DIM])
{
    u8 ids[DUCK_ACT_DIM];
    s16 pos[DUCK_ACT_DIM];              /* SyncWritePosEx modifies it */
    int i;

    (void)r;
    ids_all(ids);
    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        pos[i] = raw[i];
    }
    sms_sts.SyncWritePosEx(ids, DUCK_ACT_DIM, pos, NULL, NULL);
    return 0;
}

/* AmebaSTS_FeedBack for every joint: FeedBack(id), then ReadPos(-1) and
 * ReadSpeed(-1) (signed steps/s). The library checks each reply's
 * checksum, and a missing servo only loses its own reading. The reading
 * counts only if every servo answered. */
static int read_pos_vel_raw(duck_robot_t *r, int16_t pos[DUCK_ACT_DIM],
                            int16_t vel[DUCK_ACT_DIM])
{
    uint32_t mask = 0;
    int got = 0;
    int i;

    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        if (sms_sts.FeedBack(duck_servo_id[i]) == -1) {
            continue;
        }
        pos[i] = (int16_t)sms_sts.ReadPos(-1);
        vel[i] = (int16_t)sms_sts.ReadSpeed(-1);
        mask |= (1u << i);
        got++;
    }
    r->last_ok_mask = mask;
    return (got == DUCK_ACT_DIM) ? 0 : -1;
}

/* A broadcast reaches every servo at once; EnableTorque() to each servo
 * then repeats it with an acknowledgement. 0 when every servo acked. */
static int set_torque(duck_robot_t *r, int enable)
{
    int acked = 0;
    int i;

    write_u8_same(r, SMS_STS_TORQUE_ENABLE, enable ? 1 : 0);
    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        if (sms_sts.EnableTorque(duck_servo_id[i], enable ? 1 : 0)) {
            acked++;
        }
    }
    r->torque_on = enable;
    if (acked != DUCK_ACT_DIM) {
        duck_port_log("[bus] torque %s acknowledged by %d of %d servos",
                      enable ? "ON" : "OFF", acked, DUCK_ACT_DIM);
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int duck_robot_init(duck_robot_t *r)
{
    int i;
    int missing = 0;

    memset(r, 0, sizeof(*r));

    /* As in examples/AmebaSTS, plus a short receive timeout so a silent
     * servo stalls a control step by a few ms, not the library's 100. */
    DUCK_SERVO_SERIAL.begin(DUCK_SERVO_BAUD, SERIAL_8N1);
    sms_sts.pSerial = &DUCK_SERVO_SERIAL;
    sms_sts.IOTimeOut = DUCK_SERVO_IO_TIMEOUT_MS;

    /* The servos keep their torque and last goal across an MCU reset
     * (re-flash, crash, brown-out) as long as their own supply stays up.
     * Start from a known state: torque off, before anything else. */
    duck_robot_torque_off(r);

    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        if (sms_sts.Ping(duck_servo_id[i]) == -1) {
            duck_port_log("[bus] servo id %d not responding", duck_servo_id[i]);
            missing++;
        }
    }
    if (missing) {
        duck_port_log("[bus] %d of %d servos missing - check wiring and power",
                      missing, DUCK_ACT_DIM);
        return -1;
    }
    duck_port_log("[bus] all %d servos answered", DUCK_ACT_DIM);

    duck_port_gpio_input_pullup(DUCK_FOOT_LEFT_PIN);
    duck_port_gpio_input_pullup(DUCK_FOOT_RIGHT_PIN);
    duck_port_gpio_input_pullup(DUCK_ESTOP_PIN);
    return 0;
}

int duck_robot_set_kp(duck_robot_t *r, int kp_legs, int kp_head)
{
    uint8_t kp[DUCK_ACT_DIM];
    int i;
    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        kp[i] = (uint8_t)((i >= 5 && i <= 8) ? kp_head : kp_legs);
    }
    return write_u8_all(r, STS_P_COEF, kp);
}

void duck_robot_set_abort_hook(int (*hook)(void))
{
    s_abort_hook = hook;
}

static int abort_requested(void)
{
    if (DUCK_ESTOP_PIN >= 0 && duck_port_gpio_read(DUCK_ESTOP_PIN) == 0) {
        return 1;
    }
    return s_abort_hook ? s_abort_hook() : 0;
}

/*
 * Block until the duck has stopped moving: every joint below
 * DUCK_STILL_VEL_MAX for DUCK_STILL_HOLD_MS in a row. The servos keep
 * holding their goal meanwhile. 0 = still, -1 = not settled in time,
 * -2 = aborted (torque is off).
 */
static int wait_still(duck_robot_t *r)
{
    int16_t pos[DUCK_ACT_DIM];
    int16_t vel[DUCK_ACT_DIM];
    const uint32_t tick_ms = (uint32_t)(1000.0f / DUCK_CONTROL_HZ);
    uint32_t waited = 0;
    uint32_t still_for = 0;
    float worst = 0.0f;
    int i;

    while (still_for < DUCK_STILL_HOLD_MS) {
        if (waited >= DUCK_STILL_TIMEOUT_MS) {
            duck_port_log("[still] duck did not settle in %d ms "
                          "(fastest joint %d mrad/s)",
                          DUCK_STILL_TIMEOUT_MS, (int)(worst * 1000.0f));
            return -1;
        }
        if (abort_requested()) {
            duck_robot_torque_off(r);
            return -2;
        }
        worst = 99.0f;                      /* a failed read is not "still" */
        if (read_pos_vel_raw(r, pos, vel) == 0) {
            worst = 0.0f;
            for (i = 0; i < DUCK_ACT_DIM; ++i) {
                float v = (float)fabs(duck_speed_to_rads(vel[i]));
                if (v > worst) {
                    worst = v;
                }
            }
        }
        still_for = (worst <= DUCK_STILL_VEL_MAX) ? still_for + tick_ms : 0;
        duck_port_delay_ms(tick_ms);
        waited += tick_ms;
    }
    duck_port_log("[still] duck is standing still");
    return 0;
}

/*
 * Bring up the BNO055 the way 7Semi's examples/Basic does (begin, NDOF,
 * calibration wait), but only once the duck is standing still, so the
 * chip's gyro calibration sees no motion.
 */
int duck_robot_init_imu(duck_robot_t *r)
{
    int rc = wait_still(r);
    if (rc != 0) {
        return rc;
    }
    if (duck_imu_init(DUCK_IMU_UPSIDE_DOWN, DUCK_IMU_EXT_CRYSTAL) != 0) {
        duck_port_log("[imu] BNO055 init failed");
        return -3;
    }
    rc = duck_imu_wait_calibrated(DUCK_IMU_CALIB_TIMEOUT_MS,
                                  DUCK_IMU_CALIB_POLL_MS, abort_requested);
    if (rc == -2) {
        duck_robot_torque_off(r);
        return -2;
    }
    if (rc == 0) {
        duck_port_log("[imu] gyro not fully calibrated yet - continuing, "
                      "it keeps calibrating while the duck stands");
    }
    /* Seed the "last good sample" so a failed first read in the control
     * loop does not hand the policy zeros. */
    r->imu_consec_fail = 0;
    if (duck_imu_read(r->last_gyro, r->last_accel) != 0) {
        duck_port_log("[imu] BNO055 first read failed");
        return -4;
    }
    duck_port_log("[imu] BNO055 ready (NDOF, upside_down=%d)",
                  DUCK_IMU_UPSIDE_DOWN);
    return 0;
}

/*
 * HWI.turn_on(), made gentler. The reference jumps to the standing pose
 * under a soft gain; here the pose is ramped from wherever the joints
 * currently are, so a duck lying in an odd position does not snap.
 * The 3.5 s sequence blocks the caller, so the e-stop pin and the abort
 * hook are polled every control period.
 */
int duck_robot_turn_on(duck_robot_t *r)
{
    int16_t now_raw[DUCK_ACT_DIM];
    int16_t vel_raw[DUCK_ACT_DIM];
    double start[DUCK_ACT_DIM];
    int16_t goal[DUCK_ACT_DIM];
    int steps;
    int s;
    int i;
    const uint32_t tick_ms = (uint32_t)(1000.0f / DUCK_CONTROL_HZ);

    if (read_pos_vel_raw(r, now_raw, vel_raw) != 0) {
        duck_port_log("[on] cannot read joint positions");
        return -1;
    }

    duck_robot_set_kp(r, DUCK_KP_SOFT, DUCK_KP_SOFT);
    write_u8_same(r, STS_D_COEF, DUCK_KD);

    /* Hold where we are before enabling torque: no jump on enable. */
    write_goal_raw(r, now_raw);
    if (set_torque(r, 1) != 0) {
        /* Never stand up with a limp joint. */
        duck_robot_torque_off(r);
        return -1;
    }

    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        start[i] = duck_raw_to_rad(now_raw[i]) - k_offsets[i];
    }
    steps = (int)(DUCK_STARTUP_RAMP_S * DUCK_CONTROL_HZ);
    if (steps < 1) {
        steps = 1;
    }
    for (s = 1; s <= steps; ++s) {
        double a = (double)s / (double)steps;
        a = a * a * (3.0 - 2.0 * a);                 /* smoothstep */
        for (i = 0; i < DUCK_ACT_DIM; ++i) {
            double target = start[i] + (duck_init_pos[i] - start[i]) * a;
            goal[i] = duck_rad_to_raw(target + k_offsets[i]);
        }
        write_goal_raw(r, goal);
        duck_port_delay_ms(tick_ms);
        if (abort_requested()) {
            duck_robot_torque_off(r);
            return -2;
        }
    }

    duck_robot_set_kp(r, DUCK_KP_WALK, DUCK_KP_HEAD);
    duck_port_log("[on] standing, kp legs=%d head=%d", DUCK_KP_WALK, DUCK_KP_HEAD);
    /* RLWalk.start() sleeps 2 s after turn_on. */
    for (s = 0; s < (int)(2000u / tick_ms); ++s) {
        duck_port_delay_ms(tick_ms);
        if (abort_requested()) {
            duck_robot_torque_off(r);
            return -2;
        }
    }
    return 0;
}

void duck_robot_torque_off(duck_robot_t *r)
{
    /* Broadcast, then an acknowledged write to each servo, so one
     * corrupted frame cannot leave a joint energised. */
    set_torque(r, 0);
    duck_port_log("[safety] torque OFF");
}

int duck_robot_read(duck_robot_t *r, duck_sensors_t *s)
{
    int16_t pos[DUCK_ACT_DIM];
    int16_t vel[DUCK_ACT_DIM];
    int i;

    if (read_pos_vel_raw(r, pos, vel) != 0) {
        r->consec_fail++;
        r->total_fail++;
        return -1;
    }
    r->consec_fail = 0;

    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        s->dof_pos[i] = (float)duck_round3(duck_raw_to_rad(pos[i]) - k_offsets[i]);
        s->dof_vel[i] = (float)duck_round3(duck_speed_to_rads(vel[i]));
    }

    if (duck_imu_read(s->gyro, s->accel) != 0) {
        r->imu_fail++;
        r->imu_consec_fail++;
        /* Keep the previous IMU sample, as raw_imu.get_data() does when its
         * queue is empty. duck_robot_check_safety() cuts torque if this
         * goes on, because the fall check cannot see a stale sample. */
        memcpy(s->gyro, r->last_gyro, sizeof(s->gyro));
        memcpy(s->accel, r->last_accel, sizeof(s->accel));
    } else {
        r->imu_consec_fail = 0;
        memcpy(r->last_gyro, s->gyro, sizeof(r->last_gyro));
        memcpy(r->last_accel, s->accel, sizeof(r->last_accel));
    }

    /* Active-low switches: FeetContacts.get() returns `not pin.value`. */
    s->feet_contact[0] = (DUCK_FOOT_LEFT_PIN < 0) ? 1 :
                         (unsigned char)!duck_port_gpio_read(DUCK_FOOT_LEFT_PIN);
    s->feet_contact[1] = (DUCK_FOOT_RIGHT_PIN < 0) ? 1 :
                         (unsigned char)!duck_port_gpio_read(DUCK_FOOT_RIGHT_PIN);
    return 0;
}

int duck_robot_write_targets(duck_robot_t *r, const float targets[DUCK_ACT_DIM])
{
    int16_t raw[DUCK_ACT_DIM];
    int i;
    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        raw[i] = duck_rad_to_raw((double)targets[i] + k_offsets[i]);
    }
    return write_goal_raw(r, raw);
}

const char *duck_robot_check_safety(duck_robot_t *r, const duck_sensors_t *s)
{
    if (DUCK_ESTOP_PIN >= 0 && duck_port_gpio_read(DUCK_ESTOP_PIN) == 0) {
        return "e-stop button";
    }
    if (r->consec_fail >= DUCK_MAX_CONSEC_BUS_FAIL) {
        return "servo bus unresponsive";
    }
    if (r->imu_consec_fail >= DUCK_MAX_CONSEC_IMU_FAIL) {
        return "imu unresponsive";
    }
    if (s && s->accel[2] < DUCK_FALL_ACCEL_Z_MIN) {
        return "fall detected";
    }
    return 0;
}
