/*
 * duck_runtime.c - one control step, line-for-line with RLWalk.run().
 *
 * Order inside RLWalk.run() (scripts/v2_rl_walk_mujoco.py):
 *   1. obs = get_obs()                 <- uses the phase from the PREVIOUS step
 *   2. imitation_i += factor + offset; imitation_i %= nb_steps_in_period
 *   3. imitation_phase = [cos, sin]    <- stored for the NEXT step's obs
 *   4. action = policy.infer(obs)
 *   5. shift action history
 *   6. motor_targets = init_pos + action * action_scale
 *   7. motor_targets[5:9] = last_commands[3:] + motor_targets[5:9]
 *
 * Step 3 updating the phase only after obs is built is easy to miss and
 * shifts the whole gait by one step if done the other way round.
 */

#include "duck_runtime.h"
#include "duck_config.h"

#include <math.h>
#include <string.h>

const unsigned char duck_servo_id[DUCK_ACT_DIM] = {
    20, 21, 22, 23, 24,
    30, 31, 32, 33,
    10, 11, 12, 13, 14
};

/* HWI.init_pos in rustypot_position_hwi.py */
const float duck_init_pos[DUCK_ACT_DIM] = {
     0.002f,  0.053f, -0.630f,  1.368f, -0.784f,
     0.000f,  0.000f,  0.000f,  0.000f,
    -0.003f, -0.065f,  0.635f,  1.379f, -0.796f
};

void duck_runtime_init(duck_runtime_t *rt)
{
    int i;

    memset(rt, 0, sizeof(*rt));
    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        rt->motor_targets[i] = duck_init_pos[i];
    }
    rt->period_steps = DUCK_PERIOD_STEPS;
    rt->phase_freq_factor = 1.0f;
    rt->phase_freq_offset = DUCK_PHASE_FREQ_OFFSET;
    rt->imitation_i = 0.0f;
    /* imitation_phase starts as np.array([0, 0]), not (cos 0, sin 0). */
    rt->phase_cos = 0.0f;
    rt->phase_sin = 0.0f;
}

void duck_runtime_set_period(duck_runtime_t *rt, float period_steps)
{
    if (period_steps > 0.0f) {
        rt->period_steps = period_steps;
    }
}

void duck_runtime_build_obs(const duck_runtime_t *rt,
                            const duck_sensors_t *s,
                            const float commands[7],
                            float obs[DUCK_OBS_DIM])
{
    int i;

    for (i = 0; i < 3; ++i) {
        obs[DUCK_OBS_GYRO + i]  = s->gyro[i];
        obs[DUCK_OBS_ACCEL + i] = s->accel[i];
    }
    for (i = 0; i < 7; ++i) {
        obs[DUCK_OBS_COMMANDS + i] = commands[i];
    }
    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        obs[DUCK_OBS_DOF_POS + i] = s->dof_pos[i] - duck_init_pos[i];
        obs[DUCK_OBS_DOF_VEL + i] = s->dof_vel[i] * DUCK_DOF_VEL_SCALE;
        obs[DUCK_OBS_ACT_T1 + i]  = rt->action_t1[i];
        obs[DUCK_OBS_ACT_T2 + i]  = rt->action_t2[i];
        obs[DUCK_OBS_ACT_T3 + i]  = rt->action_t3[i];
        obs[DUCK_OBS_TARGETS + i] = rt->motor_targets[i];
    }
    obs[DUCK_OBS_FEET + 0]  = s->feet_contact[0] ? 1.0f : 0.0f;
    obs[DUCK_OBS_FEET + 1]  = s->feet_contact[1] ? 1.0f : 0.0f;
    obs[DUCK_OBS_PHASE + 0] = rt->phase_cos;
    obs[DUCK_OBS_PHASE + 1] = rt->phase_sin;
}

void duck_runtime_step(duck_runtime_t *rt,
                       const duck_sensors_t *sensors,
                       const float commands[7],
                       float motor_targets_out[DUCK_ACT_DIM])
{
    float obs[DUCK_OBS_DIM];
    float action[DUCK_ACT_DIM];
    float ph;
    int i;

    /* 1. observation with the previous phase */
    duck_runtime_build_obs(rt, sensors, commands, obs);

    /* 2-3. advance the phase for next time */
    rt->imitation_i += (rt->phase_freq_factor + rt->phase_freq_offset);
    rt->imitation_i = fmodf(rt->imitation_i, rt->period_steps);
    ph = rt->imitation_i / rt->period_steps * 6.28318530717958647692f;
    rt->phase_cos = cosf(ph);
    rt->phase_sin = sinf(ph);

    /* 4. inference */
    duck_policy_infer(obs, action);

    /* 5. history */
    memcpy(rt->action_t3, rt->action_t2, sizeof(rt->action_t3));
    memcpy(rt->action_t2, rt->action_t1, sizeof(rt->action_t2));
    memcpy(rt->action_t1, action,        sizeof(rt->action_t1));

    /* 6. targets */
    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        rt->motor_targets[i] = duck_init_pos[i] + action[i] * DUCK_ACTION_SCALE;
    }

    /* 7. head commands ride on top of the policy's head output */
    for (i = 0; i < 4; ++i) {
        rt->motor_targets[5 + i] += commands[3 + i];
    }

    memcpy(motor_targets_out, rt->motor_targets,
           DUCK_ACT_DIM * sizeof(float));
}
