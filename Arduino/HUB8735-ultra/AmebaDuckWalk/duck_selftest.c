/*
 * duck_selftest.c - numeric validation against the exported golden vectors.
 * Run this on the Ameba Pro2 before connecting any servo.
 */

#include "duck_policy.h"
#include "duck_policy_golden.h"

#include <math.h>

float duck_policy_selftest(void)
{
    float worst = 0.0f;
    int v, i;

    for (v = 0; v < DUCK_GOLDEN_COUNT; ++v) {
        float action[DUCK_ACT_DIM];
        duck_policy_infer(&duck_golden_in[v * DUCK_OBS_DIM], action);
        for (i = 0; i < DUCK_ACT_DIM; ++i) {
            const float d = fabsf(action[i] - duck_golden_out[v * DUCK_ACT_DIM + i]);
            if (d > worst) {
                worst = d;
            }
        }
    }
    return worst;
}
