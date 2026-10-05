/*
 * duck_policy.c - Open Duck Mini v2 walking policy, plain C forward pass.
 *
 * Target: Realtek Ameba Pro2 (RTL8735B), Arm v8M @ 500 MHz.
 * No ONNX runtime, no dynamic allocation, no FPU-unfriendly ops.
 *
 * Graph reproduced (from BEST_WALK_ONNX_2.onnx):
 *   x = (obs - mean) * recip
 *   x = silu(x * W0 + b0)      101 -> 512
 *   x = silu(x * W1 + b1)      512 -> 256
 *   x = silu(x * W2 + b2)      256 -> 128
 *   x =      x * W3 + b3       128 ->  28
 *   action = tanh(x[0..13])
 *
 * silu(v) = v * sigmoid(v)   (ONNX emits this as Sigmoid followed by Mul)
 */

#include "duck_policy.h"
#include "duck_policy_weights.h"

#include <math.h>
#include <string.h>

/* Two scratch buffers, ping-ponged between layers. Sized for the widest
 * tensor in the graph so no layer can overflow them. Static storage: the
 * control loop must never allocate. */
static float buf_a[DUCK_MAX_WIDTH];
static float buf_b[DUCK_MAX_WIDTH];

/* Numerically stable sigmoid: expf() of a large positive argument overflows,
 * so fold the sign into the algebra instead. */
static inline float stable_sigmoid(float v)
{
    if (v >= 0.0f) {
        return 1.0f / (1.0f + expf(-v));
    } else {
        const float e = expf(v);
        return e / (1.0f + e);
    }
}

/*
 * y[0..n_out-1] = x[0..n_in-1] * W + b
 * W is row-major [n_in][n_out] exactly as exported (ONNX transB=0).
 * The accumulation walks W row by row, which keeps the inner loop
 * sequential in memory and friendly to the cache / prefetcher.
 */
static void dense(const float *x, const float *w, const float *b,
                  int n_in, int n_out, float *y)
{
    int i, j;

    memcpy(y, b, (size_t)n_out * sizeof(float));

    for (i = 0; i < n_in; ++i) {
        const float xi = x[i];
        if (xi == 0.0f) {
            continue;
        }
        {
            const float *wrow = w + (size_t)i * (size_t)n_out;
            for (j = 0; j < n_out; ++j) {
                y[j] += xi * wrow[j];
            }
        }
    }
}

static void apply_silu(float *v, int n)
{
    int i;
    for (i = 0; i < n; ++i) {
        v[i] *= stable_sigmoid(v[i]);
    }
}

void duck_policy_infer(const float obs[DUCK_OBS_DIM],
                       float action[DUCK_ACT_DIM])
{
    float *cur = buf_a;
    float *nxt = buf_b;
    int i;

    for (i = 0; i < DUCK_OBS_DIM; ++i) {
        cur[i] = (obs[i] - duck_norm_mean[i]) * duck_norm_recip[i];
    }

    dense(cur, duck_w0, duck_b0, DUCK_L0_IN, DUCK_L0_OUT, nxt);
    apply_silu(nxt, DUCK_L0_OUT);
    cur = buf_b; nxt = buf_a;

    dense(cur, duck_w1, duck_b1, DUCK_L1_IN, DUCK_L1_OUT, nxt);
    apply_silu(nxt, DUCK_L1_OUT);
    cur = buf_a; nxt = buf_b;

    dense(cur, duck_w2, duck_b2, DUCK_L2_IN, DUCK_L2_OUT, nxt);
    apply_silu(nxt, DUCK_L2_OUT);
    cur = buf_b; nxt = buf_a;

    /* Final layer emits 28 values: the first 14 are the action mean, the
     * remaining 14 are the log-std of the training-time Gaussian and are
     * discarded at inference (the ONNX Split does the same). */
    dense(cur, duck_w3, duck_b3, DUCK_L3_IN, DUCK_L3_OUT, nxt);

    for (i = 0; i < DUCK_ACT_DIM; ++i) {
        action[i] = tanhf(nxt[i]);
    }
}
