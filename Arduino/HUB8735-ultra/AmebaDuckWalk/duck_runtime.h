/*
 * duck_runtime.h - control-loop state, mirrors RLWalk in the Python runtime.
 */
#ifndef DUCK_RUNTIME_H
#define DUCK_RUNTIME_H

#include "duck_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float gyro[3];                      /* rad/s */
    float accel[3];                     /* m/s^2, gravity included */
    float dof_pos[DUCK_ACT_DIM];        /* rad, offset-corrected, rounded 1e-3 */
    float dof_vel[DUCK_ACT_DIM];        /* rad/s, rounded 1e-3 */
    unsigned char feet_contact[2];      /* left, right; 1 = touching */
} duck_sensors_t;

typedef struct {
    float action_t1[DUCK_ACT_DIM];
    float action_t2[DUCK_ACT_DIM];
    float action_t3[DUCK_ACT_DIM];
    float motor_targets[DUCK_ACT_DIM];
    float imitation_i;
    float period_steps;
    float phase_freq_factor;
    float phase_freq_offset;
    float phase_cos;
    float phase_sin;
} duck_runtime_t;

void duck_runtime_init(duck_runtime_t *rt);
void duck_runtime_set_period(duck_runtime_t *rt, float period_steps);

/* Exposed for testing; duck_runtime_step() calls it internally. */
void duck_runtime_build_obs(const duck_runtime_t *rt,
                            const duck_sensors_t *s,
                            const float commands[7],
                            float obs[DUCK_OBS_DIM]);

/* commands = vx, vy, vyaw, neck_pitch, head_pitch, head_yaw, head_roll */
void duck_runtime_step(duck_runtime_t *rt,
                       const duck_sensors_t *sensors,
                       const float commands[7],
                       float motor_targets_out[DUCK_ACT_DIM]);

#ifdef __cplusplus
}
#endif

#endif /* DUCK_RUNTIME_H */
