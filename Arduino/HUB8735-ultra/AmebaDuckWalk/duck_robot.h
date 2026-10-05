/*
 * duck_robot.h - hardware interface (counterpart of the Python HWI class).
 */
#ifndef DUCK_ROBOT_H
#define DUCK_ROBOT_H

#include <stdint.h>
#include "duck_imu.h"
#include "duck_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int      torque_on;
    int      consec_fail;
    int      imu_consec_fail;
    uint32_t total_fail;
    uint32_t imu_fail;
    uint32_t last_ok_mask;
    float    last_gyro[3];
    float    last_accel[3];
} duck_robot_t;

/* Pure conversions, identical to rustypot + HWI. */
double  duck_raw_to_rad(int16_t raw);
int16_t duck_rad_to_raw(double rad);
double  duck_speed_to_rads(int16_t steps);    /* signed steps/s from SCServo */
double  duck_round3(double x);

int  duck_robot_init(duck_robot_t *r);          /* bus, servos, GPIO; torque off */

/* Call after duck_robot_turn_on(). Waits until no joint moves, then
 * starts the BNO055 (7Semi Basic: begin, NDOF, calibration wait).
 * 0 = ready, -1 = duck did not stand still, -2 = aborted (torque off),
 * -3 = BNO055 not found, -4 = first IMU read failed. */
int  duck_robot_init_imu(duck_robot_t *r);
int  duck_robot_set_kp(duck_robot_t *r, int kp_legs, int kp_head);

/* Polled during the blocking stand-up; return non-zero to abort it. */
void duck_robot_set_abort_hook(int (*hook)(void));

/* Ramp to the standing pose. 0 = standing, -1 = read failed,
 * -2 = aborted by the hook or the e-stop pin (torque is off again). */
int  duck_robot_turn_on(duck_robot_t *r);
void duck_robot_torque_off(duck_robot_t *r);

/* 0 on success; <0 means skip this control step (as the Python loop does). */
int  duck_robot_read(duck_robot_t *r, duck_sensors_t *s);
int  duck_robot_write_targets(duck_robot_t *r, const float targets[DUCK_ACT_DIM]);

/* Returns a reason string if torque must be cut, else NULL. */
const char *duck_robot_check_safety(duck_robot_t *r, const duck_sensors_t *s);

#ifdef __cplusplus
}

/* The servo bus (SCServo SMS_STS on DUCK_SERVO_SERIAL), opened by
 * duck_robot_init(). Shared with the sketch for joint calibration. */
#include <SCServo.h>
extern SMS_STS sms_sts;
#endif

#endif /* DUCK_ROBOT_H */
