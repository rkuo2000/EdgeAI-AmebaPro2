/*
 * duck_policy.h - public API for the Open Duck Mini v2 policy on Ameba Pro2.
 */

#ifndef DUCK_POLICY_H
#define DUCK_POLICY_H

#ifdef __cplusplus
extern "C" {
#endif

#define DUCK_OBS_DIM 101
#define DUCK_ACT_DIM 14

/*
 * Observation layout - must match the Python runtime byte for byte.
 * Source: RLWalk.get_obs() in scripts/v2_rl_walk_mujoco.py
 *
 *   index   size  contents
 *   -----------------------------------------------------------------
 *     0       3   IMU gyro            x, y, z   [rad/s]
 *     3       3   IMU accelerometer   x, y, z   [m/s^2]  (z ~ +9.8 upright)
 *     6       7   commands: vx, vy, vyaw, neck_pitch, head_pitch,
 *                 head_yaw, head_roll
 *    13      14   dof_pos - init_pos            [rad]
 *    27      14   dof_vel * 0.05                [rad/s, pre-scaled]
 *    41      14   last_action                   (raw policy output, t-1)
 *    55      14   last_last_action              (t-2)
 *    69      14   last_last_last_action         (t-3)
 *    83      14   motor_targets                 [rad, absolute]
 *    97       2   feet contact: left, right     (1.0 = touching)
 *    99       2   imitation phase: cos, sin
 *   -----------------------------------------------------------------
 *                 101 total
 *
 * Joint order for every 14-wide block:
 *   0 left_hip_yaw     1 left_hip_roll    2 left_hip_pitch
 *   3 left_knee        4 left_ankle       5 neck_pitch
 *   6 head_pitch       7 head_yaw         8 head_roll
 *   9 right_hip_yaw   10 right_hip_roll  11 right_hip_pitch
 *  12 right_knee      13 right_ankle
 */
#define DUCK_OBS_GYRO        0
#define DUCK_OBS_ACCEL       3
#define DUCK_OBS_COMMANDS    6
#define DUCK_OBS_DOF_POS    13
#define DUCK_OBS_DOF_VEL    27
#define DUCK_OBS_ACT_T1     41
#define DUCK_OBS_ACT_T2     55
#define DUCK_OBS_ACT_T3     69
#define DUCK_OBS_TARGETS    83
#define DUCK_OBS_FEET       97
#define DUCK_OBS_PHASE      99

/* Constants taken from the Python runtime defaults. */
#define DUCK_CONTROL_HZ      50.0f
#define DUCK_ACTION_SCALE    0.25f
#define DUCK_DOF_VEL_SCALE   0.05f
#define DUCK_MAX_MOTOR_VEL   5.24f   /* rad/s, used by the optional clamp */

/* Servo IDs on the Feetech bus, in the same order as the joint indices. */
extern const unsigned char duck_servo_id[DUCK_ACT_DIM];

/* Standing pose the policy was trained around, in radians. */
extern const float duck_init_pos[DUCK_ACT_DIM];

/*
 * Run one forward pass. Pure function of obs; no internal state.
 * Cost on Ameba Pro2: 219136 multiply-accumulates.
 */
void duck_policy_infer(const float obs[DUCK_OBS_DIM],
                       float action[DUCK_ACT_DIM]);

/*
 * Compare against the exported golden vectors.
 * Returns the largest absolute deviation across all vectors and outputs.
 * Anything below 1e-4 means the port is numerically correct.
 */
float duck_policy_selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* DUCK_POLICY_H */
