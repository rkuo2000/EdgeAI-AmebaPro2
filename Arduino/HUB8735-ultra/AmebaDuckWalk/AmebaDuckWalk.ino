/*
 * AmebaDuckWalk.ino - Open Duck Mini v2 RL walking on Realtek Ameba Pro2.
 *
 * Board: AMB82-MINI (RTL8735B), ameba-arduino-pro2 core.
 * Policy: BEST_WALK_ONNX_2.onnx from apirrone/Open_Duck_Mini (Apache-2.0),
 *         converted to C; behaviour matches RLWalk.run() step for step.
 *
 * Open the serial monitor at 115200 and press 'h'.
 *
 * Boot:    servos -> stand up -> wait until no joint moves
 *          -> BNO055 init + gyro calibration (7Semi Basic) -> commands
 * States:  IDLE -> (s) stand up -> stand still -> IMU init -> STANDING
 *          STANDING -> (g) WALKING -> (p) STANDING
 *          IMU init failed -> HOLD (pose held, 'u' retries the IMU)
 *          any   -> (k) IDLE      safety trip -> FAULT -> (r) IDLE
 */

#include <Arduino.h>
#include "duck_config.h"
#include "duck_policy.h"
#include "duck_runtime.h"
#include "duck_robot.h"

/* Printed at boot so a log always tells which firmware produced it. */
#define DUCK_FW_VERSION "ameba_duck_walk v9 (2026-10-05, console feedback, SCServo direct, 7Semi Basic IMU)"

/* ST_HOLD: standing with torque on, but the IMU is not up, so the control
 * loop (which needs it for the observation and the fall check) is off and
 * the servos simply hold their last goal. */
enum State { ST_IDLE, ST_STANDING, ST_WALKING, ST_FAULT, ST_CALIB, ST_HOLD };

static duck_robot_t   robot;
static duck_runtime_t rt;
static State          state = ST_IDLE;
static bool           hw_ok = false;      /* servo bus */
static bool           imu_ok = false;
static bool           policy_ok = false;
static bool           telemetry = false;

static float cmd[7] = {0, 0, 0, 0, 0, 0, 0};   /* vx vy vyaw neck head_p head_y head_r */

static const uint32_t PERIOD_US = (uint32_t)(1000000.0f / DUCK_CONTROL_HZ);
static uint32_t next_tick = 0;

/* timing statistics, microseconds */
static uint32_t loop_max = 0, loop_sum = 0, loop_n = 0, overruns = 0;
static uint32_t infer_us = 0;
static uint32_t step_count = 0;

/* ------------------------------------------------------------------ */

static void fault(const char *why)
{
    duck_robot_torque_off(&robot);
    state = ST_FAULT;
    Serial.print("[FAULT] ");
    Serial.println(why);
    Serial.println("Torque is off. Fix the cause, then press 'r'.");
}

static const char *state_name(State st)
{
    switch (st) {
    case ST_IDLE:     return "IDLE";
    case ST_STANDING: return "STANDING";
    case ST_WALKING:  return "WALKING";
    case ST_FAULT:    return "FAULT";
    case ST_CALIB:    return "CALIB";
    case ST_HOLD:     return "HOLD";
    }
    return "?";
}

/* Say why a key did nothing instead of dropping it silently. */
static void ignored(int c, const char *needs)
{
    Serial.print("'"); Serial.print((char)c);
    Serial.print("' ignored: state is "); Serial.print(state_name(state));
    Serial.print(", needs "); Serial.println(needs);
}

static float clampf(float v, float lim)
{
    return v > lim ? lim : (v < -lim ? -lim : v);
}

static void print_cmd(void)
{
    Serial.print("cmd vx=");   Serial.print(cmd[0], 3);
    Serial.print(" vy=");      Serial.print(cmd[1], 3);
    Serial.print(" vyaw=");    Serial.println(cmd[2], 3);
}

static void help(void)
{
    Serial.println();
    Serial.println("=== Ameba Duck console ===");
    Serial.println(" s  stand up, then init the IMU while standing still");
    Serial.println(" u  retry IMU init (when holding pose after an IMU failure)");
    Serial.println(" g  start walking policy      p  pause (hold pose)");
    Serial.println(" w/x  forward/back   q/e  strafe   a/d  turn");
    Serial.println(" space  zero velocity commands");
    Serial.println(" k  KILL - torque off immediately");
    Serial.println(" c  calibrate joint offsets (torque off)");
    Serial.println(" i  info / timing      t  toggle telemetry");
    Serial.println(" r  reset after a fault    h  this help");
}

static void info(void)
{
    Serial.println();
    Serial.print("state="); Serial.print(state_name(state));
    Serial.print(" hw_ok="); Serial.print(hw_ok);
    Serial.print(" imu_ok="); Serial.print(imu_ok);
    Serial.print(" policy_ok="); Serial.println(policy_ok);
    Serial.print("period steps="); Serial.print(DUCK_PERIOD_STEPS, 0);
    Serial.print(" control Hz="); Serial.println(DUCK_CONTROL_HZ, 0);
    Serial.print("inference us="); Serial.println(infer_us);
    if (loop_n) {
        Serial.print("loop avg us="); Serial.print(loop_sum / loop_n);
        Serial.print(" max us="); Serial.print(loop_max);
        Serial.print(" overruns="); Serial.print(overruns);
        Serial.print(" / steps="); Serial.println(step_count);
    }
    Serial.print("read fails total="); Serial.print(robot.total_fail);
    Serial.print(" imu fails="); Serial.print(robot.imu_fail);
    Serial.print(" last ok mask=0x"); Serial.println(robot.last_ok_mask, HEX);
    print_cmd();
}

/* Raw position of every joint, as in SCServo's AmebaSTS_ReadPos_All.
 * Bit i of the result is set when servo i answered. */
static uint32_t read_joint_raw(int16_t raw[DUCK_ACT_DIM])
{
    uint32_t mask = 0;
    for (int i = 0; i < DUCK_ACT_DIM; ++i) {
        raw[i] = 0;
        if (sms_sts.FeedBack(duck_servo_id[i]) != -1) {
            raw[i] = (int16_t)sms_sts.ReadPos(-1);
            mask |= 1u << i;
        }
    }
    return mask;
}

/*
 * Offset calibration, equivalent to scripts/find_soft_offsets.py:
 * with torque off, pose every joint at its mechanical zero (per the
 * assembly guide), then the raw reading of each joint IS its offset.
 */
static void calibrate(void)
{
    duck_robot_torque_off(&robot);
    state = ST_CALIB;
    Serial.println();
    Serial.println("=== CALIBRATION ===");
    Serial.println("Torque is off. Pose EVERY joint at its mechanical zero");
    Serial.println("(see the Open Duck Mini assembly guide), then send 'y'.");
    Serial.println("Readings refresh every second. 'y' captures, 'x' aborts.");

    /* The Serial Monitor sends "c\n" in one go; drop the line ending (and
     * anything else typed ahead) so it cannot trigger an instant capture. */
    delay(50);
    while (Serial.available()) {
        (void)Serial.read();
    }

    uint32_t last = 0;
    for (;;) {
        if (Serial.available()) {
            int c = Serial.read();
            if (c == 'y' || c == 'Y') {
                break;
            }
            if (c == 'x' || c == 'X' || c == 'k') {
                Serial.println("Calibration aborted, nothing changed.");
                state = ST_IDLE;
                return;
            }
        }
        if (millis() - last > 1000) {
            last = millis();
            int16_t raw[DUCK_ACT_DIM];
            uint32_t got = read_joint_raw(raw);
            Serial.print("rad:");
            for (int i = 0; i < DUCK_ACT_DIM; ++i) {
                Serial.print(' ');
                Serial.print((got >> i) & 1 ? (float)duck_raw_to_rad(raw[i]) : NAN, 3);
            }
            Serial.println();
        }
    }

    int16_t raw[DUCK_ACT_DIM];
    uint32_t got = read_joint_raw(raw);
    if (got != (1u << DUCK_ACT_DIM) - 1u) {
        Serial.println("Capture failed: not every servo answered. Try again.");
    } else {
        Serial.println();
        Serial.println("Paste this into duck_config.h and re-flash:");
        Serial.println();
        Serial.print("#define DUCK_JOINT_OFFSETS { ");
        for (int i = 0; i < DUCK_ACT_DIM; ++i) {
            Serial.print((float)duck_raw_to_rad(raw[i]), 4);
            Serial.print(i < DUCK_ACT_DIM - 1 ? "f, " : "f }");
        }
        Serial.println();
    }
    state = ST_IDLE;
}

/*
 * Bring the IMU up once the duck stands still, then hand over to the
 * control loop. Without an IMU the pose is held open-loop (ST_HOLD).
 * Nothing runs the policy until this has succeeded.
 */
static void start_imu_and_control(void)
{
    if (!imu_ok) {
        Serial.println("Waiting for the duck to stand still, then starting BNO055... ('k' aborts)");
        int rc = duck_robot_init_imu(&robot);
        if (rc == -2) {
            state = ST_IDLE;
            Serial.println("IMU start aborted, torque is off.");
            return;
        }
        imu_ok = (rc == 0);
    }
    if (!imu_ok) {
        state = ST_HOLD;
        Serial.println("IMU not started - holding pose. 'u' retries, 'k' releases.");
        return;
    }
    Serial.print("[imu] accel = ");
    Serial.print(robot.last_accel[0], 2); Serial.print(' ');
    Serial.print(robot.last_accel[1], 2); Serial.print(' ');
    Serial.print(robot.last_accel[2], 2);
    Serial.println(" m/s^2 (z should be ~ +9.8 upright)");

    duck_runtime_init(&rt);
    state = ST_STANDING;
    next_tick = micros();
    Serial.println("Standing. Press 'g' to walk, 'h' for help.");
}

static void stand_up(void)
{
    Serial.println("Standing up... ('k' aborts)");
    int rc = duck_robot_turn_on(&robot);
    if (rc == 0) {
        start_imu_and_control();
    } else if (rc == -2) {
        state = ST_IDLE;
        Serial.println("Stand-up aborted, torque is off.");
    } else {
        fault("could not stand up");
    }
}

static void handle_key(int c)
{
    switch (c) {
    case 'h': help(); break;
    case 'i': info(); break;
    case 't':
        telemetry = !telemetry;
        Serial.println(telemetry ? "telemetry ON (prints only while STANDING/WALKING)" :
                                   "telemetry OFF");
        break;
    case 'k':
        duck_robot_torque_off(&robot);
        if (state != ST_FAULT) {          /* a FAULT is only cleared by 'r' */
            state = ST_IDLE;
        }
        break;
    case 'r':
        if (state == ST_FAULT || !hw_ok) {
            hw_ok = (duck_robot_init(&robot) == 0);
            imu_ok = false;               /* re-initialised at the next stand-up */
            state = ST_IDLE;
            Serial.println(hw_ok ? "Hardware OK. Press 's' to stand." :
                                   "Hardware init failed, see log above.");
        } else {
            ignored(c, "FAULT or failed hardware init");
        }
        break;
    case 's':
        if (!hw_ok || !policy_ok) {
            Serial.println("Refusing: hardware or policy self-test not OK.");
        } else if (state == ST_IDLE) {
            stand_up();
        } else {
            ignored(c, "IDLE (press 'k' first)");
        }
        break;
    case 'u':
        if (state == ST_HOLD) {
            start_imu_and_control();
        } else {
            ignored(c, "HOLD");
        }
        break;
    case 'g':
        if (state == ST_STANDING) {
            state = ST_WALKING;
            next_tick = micros();
            Serial.println("Walking policy running.");
        } else {
            ignored(c, "STANDING");
        }
        break;
    case 'p':
        if (state == ST_WALKING) {
            state = ST_STANDING;
            Serial.println("Paused - holding last pose.");
        } else {
            ignored(c, "WALKING");
        }
        break;
    case 'c':
        /* Not while STANDING: calibration cuts torque and the duck would drop. */
        if (state == ST_IDLE) {
            calibrate();
        } else {
            ignored(c, "IDLE (press 'k' first)");
        }
        break;
    case 'w': cmd[0] = clampf(cmd[0] + 0.05f, DUCK_CMD_VX_MAX);   print_cmd(); break;
    case 'x': cmd[0] = clampf(cmd[0] - 0.05f, DUCK_CMD_VX_MAX);   print_cmd(); break;
    case 'q': cmd[1] = clampf(cmd[1] + 0.05f, DUCK_CMD_VY_MAX);   print_cmd(); break;
    case 'e': cmd[1] = clampf(cmd[1] - 0.05f, DUCK_CMD_VY_MAX);   print_cmd(); break;
    case 'a': cmd[2] = clampf(cmd[2] + 0.25f, DUCK_CMD_VYAW_MAX); print_cmd(); break;
    case 'd': cmd[2] = clampf(cmd[2] - 0.25f, DUCK_CMD_VYAW_MAX); print_cmd(); break;
    case ' ': cmd[0] = cmd[1] = cmd[2] = 0.0f; print_cmd(); break;
    case '\r': case '\n': break;
    default:
        Serial.print("unknown key '"); Serial.print((char)c);
        Serial.println("', 'h' for help");
        break;
    }
}

/* Polled by duck_robot_turn_on() and duck_robot_init_imu() so 'k' still
 * works during the stand-up, stillness check and IMU calibration, which
 * block loop(). Other keys typed meanwhile are dropped, and that is said. */
static int kill_pressed(void)
{
    while (Serial.available()) {
        int c = Serial.read();
        if (c == 'k') {
            return 1;
        }
        if (c != '\r' && c != '\n') {
            Serial.print("busy (stand-up / IMU start) - '");
            Serial.print((char)c);
            Serial.println("' dropped, only 'k' works until this finishes");
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */

void setup()
{
    Serial.begin(115200);
    delay(1500);
    Serial.println();
    Serial.println("Ameba Duck - Open Duck Mini v2 RL walk on AmebaPro2");
    Serial.println("firmware: " DUCK_FW_VERSION);

    /* 1. The policy must reproduce the ONNX outputs before anything moves. */
    float dev = duck_policy_selftest();
    policy_ok = dev < 1e-4f;
    Serial.print("[policy] self-test max deviation = ");
    Serial.print(dev, 8);
    Serial.println(policy_ok ? "  PASS" : "  FAIL - do not run on hardware");

    /* 2. Measure inference time on this chip. */
    {
        float obs[DUCK_OBS_DIM] = {0}, act[DUCK_ACT_DIM];
        uint32_t t0 = micros();
        for (int k = 0; k < 20; ++k) {
            obs[0] = k * 0.001f;
            duck_policy_infer(obs, act);
        }
        infer_us = (micros() - t0) / 20;
        Serial.print("[policy] inference = ");
        Serial.print(infer_us);
        Serial.print(" us of a ");
        Serial.print(PERIOD_US);
        Serial.println(" us control period");
    }

    /* 3. Servos. duck_robot_init() turns torque off first. */
    duck_robot_set_abort_hook(kill_pressed);
    hw_ok = (duck_robot_init(&robot) == 0);
    Serial.println(hw_ok ? "[hw] servos ready" : "[hw] init failed - press 'r' to retry");

    /* 4. Stand up and keep still, then 5. bring up the BNO055 while the
     * duck is motionless. Only after that are commands accepted. */
    if (hw_ok && policy_ok) {
        Serial.println("Auto stand-up in 2 s - send 'k' to cancel.");
        uint32_t t0 = millis();
        bool cancel = false;
        while (millis() - t0 < 2000 && !cancel) {
            cancel = kill_pressed();
        }
        if (cancel) {
            Serial.println("Auto stand-up cancelled. Press 's' to stand.");
        } else {
            stand_up();
        }
    }

    help();
    Serial.print("Ready, state="); Serial.println(state_name(state));
}

void loop()
{
    while (Serial.available()) {
        int c = Serial.read();
        if (c == '\r' || c == '\n') {
            continue;
        }
        handle_key(c);
        Serial.print("[state] ");
        Serial.println(state_name(state));
    }

    if (state != ST_STANDING && state != ST_WALKING) {
        return;
    }

    uint32_t now = micros();
    if ((int32_t)(now - next_tick) < 0) {
        return;
    }
    /* Like RLWalk: no catch-up after an overrun, just start the next one. */
    next_tick += PERIOD_US;
    if ((int32_t)(now - next_tick) > 0) {
        next_tick = now + PERIOD_US;
    }

    uint32_t t0 = micros();
    duck_sensors_t s;

    if (duck_robot_read(&robot, &s) != 0) {
        /* get_obs() returned None: skip this step entirely, as Python does. */
        const char *why = duck_robot_check_safety(&robot, 0);
        if (why) {
            fault(why);
        }
        return;
    }

    const char *why = duck_robot_check_safety(&robot, &s);
    if (why) {
        fault(why);
        return;
    }

    if (state == ST_WALKING) {
        float targets[DUCK_ACT_DIM];
        duck_runtime_step(&rt, &s, cmd, targets);
        duck_robot_write_targets(&robot, targets);
        step_count++;
    }

    uint32_t took = micros() - t0;
    loop_sum += took;
    loop_n++;
    if (took > loop_max) loop_max = took;
    if (took > PERIOD_US) {
        overruns++;
        if (overruns <= 5 || overruns % 50 == 0) {
            Serial.print("[timing] control budget exceeded by ");
            Serial.print(took - PERIOD_US);
            Serial.println(" us");
        }
    }

    /* loop_n advances in STANDING too; step_count does not, and would print
     * every 20 ms while standing. */
    if (telemetry && (loop_n % 25 == 0)) {
        Serial.print("t="); Serial.print(step_count);
        Serial.print(" loop_us="); Serial.print(took);
        Serial.print(" az="); Serial.print(s.accel[2], 2);
        Serial.print(" feet="); Serial.print(s.feet_contact[0]);
        Serial.print(s.feet_contact[1]);
        Serial.print(" phase="); Serial.println(rt.imitation_i, 1);
    }
}
