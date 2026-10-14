
# Agentic Robot Proposal: Google Edge AI + Gemma + AMB82-Mini

## Original Prompt

> I would like to design an agentic robot with Google Edge AI Gemma4-E2B model using AMB82-mini as robot controller. AMB82-Mini offer robot features such as image capture, audio/sound recording, play  mp3 files (preloaded in SDcard, or receive voice file from TTS), IR distance sensor, IMU, Wheel motor driver. There are different ways for user to communicate with the robot, one is using Edge AI App with Gemma4-E2B model to interface with the robot, the other is talking to the robot directly. Compare these two interactive ways for 1) RoboCar (two-wheel robot) 2) AI Plush Pet

---

## 1. Design Concept

This project uses a split-agent architecture:

- **Phone/tablet:** runs Google Edge AI, Gemma, multimodal reasoning, conversation, planning, and tool selection.
- **AMB82-Mini:** acts as the real-time robot body/controller for camera capture, audio recording, MP3/TTS playback, IR distance sensing, IMU, wheel/servo control, local safety, and robot actions.
- **Wi-Fi interface:** connects the AI agent to the physical robot through an HTTP, WebSocket, JSON-RPC, or MCP-style robot tool interface.

The important UX distinction is not only where Gemma executes, but **where the user feels the intelligence lives**.

### Interaction A — Edge AI App

```text
User
 │ voice / text / image
 ▼
Android Phone / Tablet
 │
Google Edge AI App
 │
Gemma Agent
 │ tool calls
 ▼
Wi-Fi
 │
AMB82-Mini
 ├─ Camera
 ├─ Microphone
 ├─ IR distance
 ├─ IMU
 ├─ Motors / servos
 └─ MP3 / TTS playback
```

The phone/tablet is visibly the AI interface.

### Interaction B — Talk Directly to Robot

```text
User
 │ speech
 ▼
Robot Microphone
 │
AMB82-Mini
 │ audio / image / sensor data
 ▼
Wi-Fi
 │
Phone / Tablet + Gemma Agent
 │ decision / tool call / TTS
 ▼
AMB82-Mini
 ├─ Speaker
 ├─ Motors / servos
 └─ LEDs / expressions
```

Gemma can still run on the phone/tablet, but the phone becomes an invisible compute module. To the user, the robot itself appears to listen, see, think, speak, and act.

---

# 2. RoboCar — Two-Wheel Agentic Robot

## Interaction Comparison

| Aspect | Edge AI App | Talk Directly to RoboCar |
|---|---|---|
| Primary UI | Phone/tablet | Robot itself |
| Speech input | Phone microphone | AMB82 robot microphone |
| Camera | Phone and/or AMB82 camera | AMB82 camera |
| Gemma inference | Phone/tablet | Phone/tablet through robot |
| Conversation visible | Yes | Usually no |
| Natural robot interaction | Medium | High |
| Debugging | Excellent | More difficult |
| Driving commands | Excellent | Excellent |
| Visual navigation | Excellent | Excellent |
| Education/development | Excellent | Very good |
| Autonomous-robot feeling | Moderate | High |
| Implementation difficulty | Lower | Higher |

## Recommended RoboCar Architecture

During development, use the **Edge AI App as the primary interface**.

```text
                ┌────────────────────────────┐
                │ Android Phone / Tablet     │
User ──────────►│ Google Edge AI App         │
 voice/text     │ Gemma Agent                │
                │ Planning + Tool Calling    │
                └─────────────┬──────────────┘
                              │ Wi-Fi
                     HTTP / WebSocket / MCP
                              │
                 ┌────────────▼─────────────┐
                 │       AMB82-Mini         │
                 │                          │
 Camera ────────►│ Image capture            │
 IR ────────────►│ Distance / obstacle       │
 IMU ───────────►│ Orientation / motion      │
                 │                          │
                 │ Robot Control API        │
                 └──────┬─────────┬─────────┘
                        │         │
                   Left Motor  Right Motor
```

Gemma should issue **semantic robot commands**, rather than raw PWM commands.

Example robot tools:

```text
move_forward(speed, duration)
move_backward(speed, duration)
turn_left(angle)
turn_right(angle)
stop()

capture_image()
get_distance()
get_imu()

record_audio()
play_sound(sound_id)
speak(text)

robot_status()
```

Example agent task:

```text
User: "Go forward and find my backpack."

Gemma:
  capture_image()
       ↓
  backpack not found
       ↓
  get_distance()
       ↓
  move_forward(slow)
       ↓
  capture_image()
       ↓
  backpack detected
       ↓
  stop()
       ↓
  speak("I found your backpack.")
```

### Safety Principle

Gemma must **not** have final authority over safety-critical motion.

The AMB82-Mini should maintain a local safety layer:

```text
Gemma command
     │
     ▼
AMB82 Safety Supervisor
     │
     ├── obstacle too close → STOP
     ├── excessive tilt → STOP
     ├── communication timeout → STOP
     └── safe → execute command
```

This means a `move_forward()` request can be rejected locally if the IR sensor detects an obstacle.

## Direct-Voice RoboCar

For a finished robot, direct voice can be added:

```text
"Robot, come here."
        │
        ▼
 AMB82 Microphone
        │
        ▼
 Phone / Gemma
        │
   Robot Agent
        │
        ▼
 AMB82 Motion
```

This gives a much stronger autonomous-robot experience while preserving the phone as the AI compute engine.

### RoboCar Recommendation

**Development / education:** Edge AI App first.

**Finished interactive robot:** Edge AI App + direct robot speech.

The app remains valuable for telemetry, camera display, debugging, manual driving, configuration, and agent logs.

---

# 3. AI Plush Pet

For an AI Plush Pet, direct robot interaction should be the primary experience.

Example:

```text
User:
"Giraffe, what are you looking at?"

        │
        ▼
AMB82 microphone + camera
        │
        ▼
Phone / Gemma multimodal agent
        │
        ▼
"I can see your red toy car."
        │
        ▼
TTS
        │
        ▼
AMB82 speaker
```

The user does not need to touch the phone.

## Interaction Comparison

| Aspect | Edge AI App | Direct Plush Interaction |
|---|---|---|
| Natural interaction | ★★★ | ★★★★★ |
| Character presence | ★★ | ★★★★★ |
| Visible phone required | Yes | No |
| Speech input | Phone microphone | Plush microphone |
| Vision | Phone/robot camera | Plush camera |
| Physical reactions | Good | Excellent |
| Character/personality illusion | Medium | Very strong |
| Configuration | Excellent | Limited |
| Debugging | Excellent | Limited |
| Developer/guardian controls | Excellent | Secondary |
| Main user experience | Secondary | Primary |

For a plush character, requiring the user to speak into a phone weakens the feeling that the character itself is alive and responsive.

Therefore:

**Direct interaction = primary user interface**

**Edge AI App = backstage control interface**

The app can provide:

- setup and Wi-Fi configuration
- personality selection
- diagnostics
- camera preview
- sensor status
- conversation/agent logs
- audio library management
- developer mode
- firmware/skill configuration

---

# 4. Recommended Dual-Interface Architecture

Rather than choosing only one method, support both.

```text
                       GEMMA ROBOT AGENT
                     Android Phone/Tablet
              ┌───────────────────────────────┐
              │                               │
 User/App ───►│ Chat / Voice / Control UI     │
              │                               │
 Robot ──────►│ Audio / Image / Sensor Input  │
              │                               │
              │            Gemma              │
              │              │                │
              │        Agent Planner          │
              │              │                │
              │         Robot Tools           │
              └──────────────┬────────────────┘
                             │
                       Wi-Fi / MCP
                             │
              ┌──────────────▼───────────────┐
              │          AMB82-Mini          │
              │                              │
              │ Camera          Microphone   │
              │ IR Sensor       IMU          │
              │                              │
              │       Robot Skill Layer      │
              │                              │
              │ Motor/Servo      Audio       │
              │ Driver           MP3/TTS     │
              └──────────────────────────────┘
```

The same agent receives two classes of events:

```text
APP EVENTS
 ├─ user_text
 ├─ user_voice
 └─ app_command
        │
        ├──────────────► GEMMA AGENT
        │
ROBOT EVENTS
 ├─ robot_audio
 ├─ robot_camera
 ├─ IR_distance
 └─ IMU
        │
        ┘
```

Both interfaces share one robot tool layer:

```text
RobotTools

vision.capture()
vision.describe()

sensor.distance()
sensor.imu()

motion.forward()
motion.backward()
motion.turn()
motion.stop()

audio.record()
audio.play()
audio.speak()

robot.status()
```

---

# 5. Recommended Intelligence Distribution

| Function | RoboCar | AI Plush Pet |
|---|---|---|
| Gemma inference | Phone/tablet | Phone/tablet |
| Agent planning | Phone/tablet | Phone/tablet |
| Multimodal reasoning | Phone/tablet | Phone/tablet |
| Conversation | Secondary | Primary |
| Camera capture | AMB82 | AMB82 |
| Audio capture | AMB82 | AMB82 |
| IR processing | AMB82 | AMB82 |
| IMU processing | AMB82 | AMB82 |
| Motor/servo control | AMB82 | AMB82 |
| Safety interlock | AMB82 | AMB82 |
| TTS generation | Phone/tablet | Phone/tablet |
| TTS playback | AMB82 | AMB82 |
| Preloaded sounds | SD card | SD card |
| Character expressions | Optional | Important |

---

# 6. Low-Latency Audio and Personality

The AI Plush Pet should not rely on generated TTS for every sound.

Store frequently used emotional sounds on the AMB82 SD card:

```text
/audio/
  hello.mp3
  laugh.mp3
  wow.mp3
  curious.mp3
  thinking.mp3
  happy.mp3
  sleepy.mp3
  yes.mp3
  no.mp3
```

Gemma can return a structured response:

```json
{
  "speech": "That's a really big red ball!",
  "emotion": "excited",
  "action": "look_left"
}
```

AMB82 can immediately:

1. play a short local emotional sound,
2. animate the head/ears/LEDs,
3. wait for the generated TTS stream/file,
4. play the full spoken response.

This hides AI/TTS latency and makes the plush feel much more responsive.

---

# 7. Different Agent Loops

## RoboCar Agent

```text
PERCEIVE
camera + IR + IMU
      │
      ▼
UNDERSTAND
Gemma multimodal reasoning
      │
      ▼
PLAN
find / follow / explore / navigate
      │
      ▼
ACT
AMB82 robot tools
      │
      ▼
VERIFY
camera + sensors
      │
      └───────────► repeat
```

The RoboCar is primarily:

**Perception → Planning → Physical Action → Verification**

## AI Plush Pet Agent

```text
LISTEN
microphone
    │
    ▼
SEE
camera
    │
    ▼
UNDERSTAND
Gemma
    │
    ▼
PERSONALITY / CONTEXT
    │
    ▼
RESPOND
speech + sound
    │
    ▼
EXPRESS
head / ears / LEDs / motion
```

The Plush Pet is primarily:

**Conversation → Context → Personality → Expression**

---

# 8. One Reusable AMB82 Robot Platform

A common robot API can support multiple products:

```text
                  Google Edge AI
                      Gemma
                        │
                 Agent Framework
                        │
          ┌─────────────┴─────────────┐
          │                           │
    RoboCar Skill               Plush Pet Skill
          │                           │
    navigation                  conversation
    exploration                 personality
    object search               expression
    follow target               interaction
          │                           │
          └─────────────┬─────────────┘
                        │
                 AMB82 Robot API
                        │
       ┌────────┬───────┼───────┬────────┐
     Camera    Audio     IR     IMU     Motion
                        │
                   AMB82-Mini
```

This allows the same AMB82 firmware and communication protocol to support different Gemma agent personalities and skills.

---

# 9. Final Recommendation

## RoboCar

Use the **Edge AI App as the primary development and educational interface**, because it provides excellent visibility into the camera, sensor data, Gemma reasoning, tool calls, robot status, and manual controls.

Add **direct robot speech** for the finished autonomous experience.

## AI Plush Pet

Use **direct conversation with the plush as the primary interface**.

The Edge AI App should operate as a secondary backstage interface for configuration, diagnostics, personality setup, logs, media management, and development.

## Overall Architecture

The most flexible platform is therefore:

```text
            USER
        ┌────┴─────┐
        │          │
    Edge App    Robot Voice
        │          │
        └────┬─────┘
             ▼
       GEMMA AGENT
             │
        Robot Skills
             │
      Wi-Fi / MCP/API
             │
        AMB82-Mini
             │
 ┌──────┬────┼────┬───────┐
Camera Audio IR   IMU    Motors
```

**Phone/tablet = reasoning brain**

**AMB82-Mini = perception, real-time control, safety, and physical embodiment**

This separation gives the RoboCar strong agentic autonomy while giving the AI Plush Pet a natural character-centered interaction model.
