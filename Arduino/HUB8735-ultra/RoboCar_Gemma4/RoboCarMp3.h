#pragma once
#include <stdint.h>

// MP3 player for the HUB8735 audio codec, replacing File::playMp3().
//
// File::playMp3() (libraries/Multimedia/src/helix_mp3_drv.c) feeds the DMA with
// xQueueSend(..., portMAX_DELAY): if the audio TX interrupt stops mid-sentence the
// speech is chopped and the call blocks forever (loop() never runs again). It also
// creates a 23 KB queue on every call and never deletes it.
// This version uses the same decoder + HAL calls, but every wait is bounded, the
// queue is created once, and the result says what happened.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int frames;           // MP3 frames decoded and queued
    int sampleRate;       // from the first frame (0 = no valid frame)
    uint32_t txIrqs;      // audio TX DMA interrupts seen (0 = DMA never ran)
    int stalled;          // 1 = TX DMA stopped consuming, playback aborted
    int noMemory;         // 1 = queue / decoder allocation failed
} RoboMp3Result;

// Blocks until playback ends (or stalls for more than stallMs). vol: 0..0xAF (0xAF = 0 dB).
RoboMp3Result roboPlayMp3(const uint8_t *data, uint32_t len, uint8_t vol, uint32_t stallMs);

#ifdef __cplusplus
}
#endif
