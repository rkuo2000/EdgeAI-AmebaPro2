# AI PlushPet on AMB82-MINI

`ILI9341_Giraffe.ino` implements the direct-interaction PlushPet portion of
`PROPOSAL_AI_RoboCar_PlushPet.md`. The AMB82 is the microphone/camera/display/speaker body; 
the configured phone or LAN server supplies the conversation model.

## Setup

1. Edit `PlushPetConfig.h`: set Wi-Fi SSID/password, AI host, port, model and API
   key. No credentials have been copied from neighboring sketches. The host is
   an IP/name without `http://`. The default endpoint is `/v1/chat/completions`.
2. For the Edge Gallery workflow used in the local `Ollama_sendAudio` example,
   initialize your model in AI Chat, open the **Text / image / audio API** dialog,
   keep it open, and copy its IP and bearer key. Use a model/server accepting
   AAC-in-MP4 `input_audio`. An OpenAI-compatible endpoint supporting text/vision
   does not by itself guarantee MP4 audio support. Set `PET_ENABLE_AUDIO=0` for
   text/vision-only use. Set the model name to one actually installed on your
   server. Joint image/audio is opt-in with `PET_IMAGE_WITH_AUDIO=1`.
3. Insert an SD card. Put optional short reaction MP3s in `/audio/`:
   `hello.mp3`, `laugh.mp3`, `wow.mp3`, `curious.mp3`, `thinking.mp3`, `happy.mp3`,
   `sleepy.mp3`, `yes.mp3`, `no.mp3`. Missing sounds are skipped.
4. Keep the original ILI9341 wiring: reset=5, DC=4, CS=`SPI_SS`, default SPI bus,
   20 MHz. The camera uses JPEG channel 0 at 640×480. Audio preset 0 selects the
   analog microphone at 8 kHz; presets 1/2/3 select 16 kHz analog/8 kHz PDM/16 kHz
   PDM. Speaker playback follows the SDK's audio jack/analog output arrangement.
5. Optional: set `PET_TOUCH_PIN` to a verified free GPIO for an active-low button
   or compatible touch output. It defaults to disabled (`-1`). Do not share pins
   with the TFT, SPI bus, SD, camera or microphone.
6. Select **Realtek AMB82-MINI** in Arduino IDE (tested with installed core
   `4.1.1-build20260922`) and compile/upload. Open Serial Monitor at 115200 baud,
   with **Newline**, **Carriage return**, or **Both NL & CR** termination (not
   **No line ending**). The sketch prints the board IP when Wi-Fi connects.

## Behavior

- Silent mic loopback supplies `micLevel()`. Two consecutive samples above 7000
  trigger a five-second audio-only MP4 recording. Tune `PET_TRIGGER_LEVEL`,
  `PET_TRIGGER_SAMPLES`, and `PET_RECORD_SECONDS` for your microphone.
- This is sound-level triggering, not wake-word recognition or speech endpoint
  detection. Recording starts after detection; there is no pre-roll buffer, so
  pause briefly after getting Giraffe's attention to avoid losing speech onset.
- Recording runs through `Audio → AAC → MP4`, using `STORAGE_AUDIO`. The camera
  is initialized lazily for an image request, independently of recording.
- Clips use the fixed filename `PetClip.mp4`, replacing the previous clip on
  each recording. Successful AI requests delete their clips by default. Failed
  clips remain on SD until the next recording. The SD host stays mounted for
  subsequent recording.
- `Ollama_sendText`, `Ollama_sendImage`, and `Ollama_sendAudio` share a request
  path. Requests set `stream:false`, escape text through ArduinoJson, and send
  media as Base64 in 768-byte blocks with correct final padding. Media is limited
  to 1 MB; responses to 32 KB. Short writes and incomplete responses fail cleanly.
- Three previous exchanges provide bounded context. Audio history records that
  the user spoke and the model's answer; it does not fabricate a transcript.
- The model is prompted to return `speech`, `emotion`, `action`, and `sound`.
  Plain-text replies and fenced JSON are also accepted. Speech is printed on
  Serial, exposed through status, and shown in four wrapped lines at the bottom
  of the TFT (white text on black). Short replies sit on the bottom line; longer
  replies scroll upward one line every 1.5 seconds while the main loop runs.
  Scrolling pauses during blocking inference, TTS download and audio playback.
  Automatic face changes preserve the text. Starting a new control command
  clears it and restores the full portrait, including when an `emotion` command
  selects the face already displayed. The `speak` command also
  displays its text. The SDK font supports ASCII only: non-ASCII characters
  appear as `?` on the TFT; Serial, status and TTS retain the original text.
  Emotions/actions select the existing seven
  giraffe portraits. Sound IDs are allowlisted; model text cannot select arbitrary
  SD paths. A curious face and local `thinking.mp3` provide waiting feedback.
- Recording links and mic monitoring stop before SD MP3 playback takes over the
  audio codec. Listening restarts after a two-second cooldown to reduce feedback.
- Seven portraits are stored losslessly in `PlushPetFaces.h`; the original
  RGB565 headers remain the source assets. Compression is necessary to fit the
  SDK's 4 MB firmware partition with camera/audio/Wi-Fi code. Run
  `python3 pack_faces.py` after editing source assets. It verifies exact pixels.

## Backstage commands

Serial commands:

```text
status
text Hello Giraffe, tell me a short story.
speak 你好！我是長頸鹿，很高興見到你。
image What do you see in front of you?
record
play laugh
emotion shy
listen off
listen on
clear
```

`listen off` disables automatic sound triggers; explicit `record` still works.
While it is off, `status` reports `State=idle`: the pet still accepts commands but
ignores sound.
For a text test, send `listen off`, then `text Hello Giraffe!` as separate lines.
Serial prints `[Command] Accepted: text` when the command is queued, followed by
`[AI] Request to ...` showing the configured server and model. If neither appears,
check 115200 baud, the line ending, and the `text ` prefix. If an AI error follows,
send `status` to inspect the last error and verify the server IP, port, model and
API key in `PlushPetConfig.h`. Commands are rejected while recording; inference
and playback pause Serial processing until they finish.
`clear` clears the three-turn conversation and diagnostic strings. Valid
emotions are `neutral`, `happy`, `excited`, `curious`, `shy`, `sleepy`.

The board also serves HTTP on port 8080:

Enter a command directly as a URL in your browser or HTTP client's URL field
(replace `BOARD_IP` with the board address):

```text
http://BOARD_IP:8080/command?Hello+Giraffe
http://BOARD_IP:8080/command?happy
http://BOARD_IP:8080/command?text=Hello+Giraffe
http://BOARD_IP:8080/command?emotion=happy
http://BOARD_IP:8080/command?speak=Hello
http://BOARD_IP:8080/command?image=What+do+you+see%3F
http://BOARD_IP:8080/command?record
http://BOARD_IP:8080/command?play=laugh
http://BOARD_IP:8080/command?listen=off
http://BOARD_IP:8080/command?clear
```

Query commands accept GET or POST and need no body. Send one command per URL.
Bare values such as `happy`, `shy`, or `curious` select an emotion; `record`,
`clear`, and `image` run those commands. Other bare values are sent to the LLM.
Use `text=happy` to send the word "happy" to the LLM instead of selecting a face.
Encode spaces as `+` or `%20`, literal `+` as `%2B`, and `&` as `%26`.
Chinese text can be UTF-8 percent-encoded by the browser/client.
The original POST JSON format is also supported:

```sh
curl http://BOARD_IP:8080/status
curl -X POST http://BOARD_IP:8080/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"text","value":"Hello Giraffe!"}'
```

Commands need no key, so anyone on the LAN can send them.
`GET /status` is available on the LAN and returns state, mic level, SD availability,
auto-listen setting, last speech/error, TTS enablement and TTS language. Command names/values
match the Serial interface. Request bodies require Content-Length and are limited
to 2048 bytes; values to 1024 bytes. Accepted commands return 202 and execute in
the loop; busy or invalid commands return 409. Poll `/status` for results.

Listening and recording are cooperative states. AI inference, TTS download and
SDK MP3 playback are synchronous: the HTTP/Serial control loop pauses while they
run. AI HTTP inference has a 120-second response deadline and uploads a
15-second no-progress deadline. There is no interrupt/stop command during those
operations. Wi-Fi is retried periodically when disconnected.
Google TTS uses the vendor library's own blocking download loop and timeout.
The vendor camera initialization/snapshot and MP3 playback calls have no sketch
timeout; a camera/codec failure can still block inside the SDK. Disable camera
requests while diagnosing a missing or unsupported camera sensor.

## Google TTS speech

Generated speech is enabled by default with the AMB82 `GenAI` library:

```cpp
#include "GenAI.h"
GenAI tts;

tts.googletts(mp3Filename, text, "zh-TW");
delay(500);
sdPlayMP3(mp3Filename);
```

The sketch passes each AI reply's `speech` to this flow. Google TTS downloads
MP3 audio directly from `translate.google.com`; the board needs internet access
and an SD card. No separate LAN TTS bridge or TTS API key is configured.
`PET_TTS_LANGUAGE` defaults to `zh-TW` (Traditional Chinese, Taiwan); set it to
`en-US` or another language supported by your GenAI TTS setup if needed.
Set `PET_ENABLE_TTS=0` to use only portraits, local sounds and Serial/status text.
Test TTS independently of the AI with `speak 你好！我是長頸鹿。` in Serial Monitor
(or a `speak` HTTP command).

Long replies are split into short requests using `PET_TTS_CHUNK_BYTES` (180 bytes
by default), respecting UTF-8 character boundaries and preferring spaces where
available. Each chunk is downloaded, played, and removed before the next chunk.
Google TTS follows the model's emotion/local sound response.

The SDK uses `temp.MP3` and calls `fs.end()` inside `googletts()`. The sketch
removes old temporary/output files before every download, remounts the shared SD
host before playback, and leaves it mounted for the next recording. `PetTTS.mp3`
and `temp.MP3` are reserved temporary files for this feature. The SDK TTS method
returns no success flag; missing/empty/oversized output is detected before
playback, but server errors in a nonempty file may require checking SDK logs.


## Scope and validation

Implemented: microphone trigger/record/upload, camera capture/upload, text chat,
personality/context, expression portraits, local emotional sounds, Google TTS
download/playback, optional touch reaction, and app/backstage command/status API.

No IR sensor, IMU, motors, head/ear servos or OTA updater are configured in this
project. They require actual hardware models, wiring and pin allocation. `wave`,
`wink`, `cheer` affect the display; they do not drive physical actuators. This
firmware does not turn the phone's app into an autonomous tool-calling agent.

Validation completed using the installed Realtek core: full Arduino compile,
link and flash-image packaging. The initial uncompressed build exceeded the
4 MB firmware region; the packed build fits. The packaged image reports
4,788,224 bytes including SDK camera/system images. No board has been flashed or
live camera, microphone, SD, speaker, server or TTS interaction tested here.
The firmware region itself is 4,079,616 of 4,194,304 bytes.

The initial implementation passed host tests for HTTP framing, media encoding,
and exact portrait decoding. The Google TTS change passed a native test of the
actual `nextTtsChunk` function with English, Traditional Chinese and emoji,
checking exact byte preservation, chunk size and UTF-8 boundaries under address
and undefined-behavior sanitizers. Hardware Google TTS download/playback and
recording after TTS still need validation on the board.

Regenerate and verify the packed portraits with `python3 pack_faces.py`.

## Reference examples

- `../Ollama_sendAudio/Ollama_sendAudio.ino`: audio-only AAC/MP4 path, Edge Gallery
  input_audio schema, unique clip filenames and keeping SD mounted.
- `../Ollama_sendImage/Ollama_sendImage.ino`: JPEG camera capture and image_url.
  Its original three-byte Base64 loop can read past the final frame; this sketch
  sends bounded blocks instead.
- `../Ollama_sendText/Ollama_sendText.ino`: shared chat-completions endpoint.
- Installed `Multimedia/examples/Audio/AudioTriggerRecording`: mic-level trigger.
- Installed `NeuralNetwork/examples/MultimediaAI/TextToSpeech` and
  `../TextToSpeech/TextToSpeech.ino`: GenAI Google TTS and SD MP3 playback.
- Installed `Multimedia/examples/RecordMP4/SingleVideoWithAudio_MP3Prompts` and
  `../SDCardPlayMP3/SDCardPlayMP3.ino`: audio codec handoff and SD playback.
- [Ameba audio trigger recording guide](https://ameba-doc-arduino-sdk.readthedocs-hosted.com/en/latest/ameba_pro2/amb82-mini/Example_Guides/Multimedia/Audio/Audio%20Trigger%20Recording.html)
- [Ollama OpenAI compatibility: text and vision chat](https://docs.ollama.com/api/openai-compatibility)
