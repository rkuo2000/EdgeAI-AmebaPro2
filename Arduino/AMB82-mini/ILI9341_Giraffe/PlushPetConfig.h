#pragma once

// Fill these in before enabling AI requests. Do not copy credentials from examples.
#define PET_WIFI_SSID "HITRON-DF90-2.4G"
#define PET_WIFI_PASSWORD "0972211921"
#define PET_AI_HOST "123.192.49.97"
#define PET_AI_PORT 11434
#define PET_AI_PATH "/v1/chat/completions"
#define PET_AI_MODEL "gemma4:e2b"
#define PET_AI_KEY "ollama"

// Audio input format sent as input_audio:
//   1 = 16-bit PCM WAV captured in RAM (Ollama / llama.cpp: WAV, MP3, FLAC only)
//   0 = AAC in MP4 on the SD card (Edge Gallery on Android, as in Ollama_sendAudio)
#define PET_AUDIO_WAV 1
#define PET_ENABLE_AUDIO 1
#define PET_ENABLE_CAMERA 1
#define PET_IMAGE_WITH_AUDIO 0  // Enable if the model supports joint audio + image input.
#define PET_AUDIO_PRESET 0     // 0=8k analog, 1=16k analog, 2=8k PDM, 3=16k PDM
// Analog mic preamp (presets 0/1). The SDK default is 0 dB; each +20 dB is about
// 10x the [Mic] peak. Raise it if speech peaks stay near the idle noise level.
#define PET_MIC_BOOST_DB 0     // 0, 20, 30 or 40; the SDK trigger example uses 0
#if PET_MIC_BOOST_DB != 0 && PET_MIC_BOOST_DB != 20 && PET_MIC_BOOST_DB != 30 && PET_MIC_BOOST_DB != 40
#error "PET_MIC_BOOST_DB must be 0, 20, 30 or 40"
#endif
#define PET_TRIGGER_LEVEL 7000
#define PET_TRIGGER_SAMPLES 2  // consecutive 100 ms samples over threshold
#define PET_MIC_DEBUG 1        // 1 = print the mic peak every second while listening
#define PET_RECORD_SECONDS 3   // 3 seconds for faster interactive
#define PET_COOLDOWN_MS 2000UL
#define PET_IP_DISPLAY_MS 5000UL  // How long setup shows the IP address on the TFT
#define PET_RESPONSE_TIMEOUT_MS 120000UL
#define PET_MAX_MEDIA_BYTES (1024UL * 1024UL)
#define PET_MAX_REPLY_BYTES 32768UL
#define PET_MP3_VOLUME 150     // SDK range 0..175
#define PET_DELETE_UPLOADED_CLIPS 1

// Google Translate TTS through the installed AMB82 GenAI library.
// Requires internet access and SD storage; no separate TTS bridge/API key.
#define PET_ENABLE_TTS 1
#define PET_TTS_LANGUAGE "en"
#define PET_TTS_FILENAME "PetTTS.mp3"
#define PET_TTS_CHUNK_BYTES 180  // UTF-8-safe short requests; minimum 4 bytes
#if PET_TTS_CHUNK_BYTES < 4
#error "PET_TTS_CHUNK_BYTES must be at least 4 for UTF-8 text"
#endif

// HTTP control API. No authorization: anyone on the LAN can send commands.
#define PET_CONTROL_PORT 8080

// Optional active-low touch/button. Avoid TFT/SPI, camera, SD and audio pins.
#define PET_TOUCH_PIN 11 // A0
