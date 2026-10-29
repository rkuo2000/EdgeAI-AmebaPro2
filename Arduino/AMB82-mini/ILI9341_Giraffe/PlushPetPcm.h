#pragma once
#include <Arduino.h>
#include "VideoStream.h"
extern "C" {
#include "mmf2_module.h"
}

// Minimal MMF sink that captures raw 16-bit PCM from Audio into a RAM WAV file.
// llama.cpp-based servers (Ollama) decode only WAV/MP3/FLAC, not AAC-in-MP4.
// For an output module with MM_TYPE_NONE, the SISO pumper task calls
// handle(priv, inputItem, NULL) and recycles the item after it returns,
// so each frame is copied synchronously from inputItem->data_addr.
class PetPcmRecorder : public MMFModule {
public:
    static const size_t HEADER_BYTES = 44;

    bool begin(uint32_t sampleRate, uint16_t channels, uint32_t seconds)
    {
        if (_p_mmf_context) return true;
        rate = sampleRate; channelCount = channels;
        capacity = (size_t)sampleRate * channels * 2 * seconds;
        buffer = (uint8_t *)malloc(HEADER_BYTES + capacity);
        if (!buffer) return false;
        instance = this;
        _p_mmf_context = mm_module_open(&module);
        if (!_p_mmf_context) { free(buffer); buffer = nullptr; return false; }
        return true;
    }
    // Call before starting the StreamIO link; frames arriving afterwards are kept.
    void arm() { length = 0; armed = true; }
    // Call after stopping the StreamIO link; returns the complete WAV size.
    size_t finish()
    {
        armed = false;
        writeHeader();
        return HEADER_BYTES + length;
    }
    bool full() const { return length >= capacity; }
    size_t pcmBytes() const { return length; }
    const uint8_t *wav() const { return buffer; }

private:
    uint8_t *buffer = nullptr;
    size_t capacity = 0;
    volatile size_t length = 0;
    volatile bool armed = false;
    uint32_t rate = 8000;
    uint16_t channelCount = 1;

    static PetPcmRecorder *instance;
    static mm_module_t module;

    void put16(size_t at, uint16_t v) { buffer[at] = v; buffer[at + 1] = v >> 8; }
    void put32(size_t at, uint32_t v) { put16(at, v); put16(at + 2, v >> 16); }
    void writeHeader()
    {
        uint32_t data = length;
        memcpy(buffer, "RIFF", 4); put32(4, 36 + data);
        memcpy(buffer + 8, "WAVEfmt ", 8); put32(16, 16);
        put16(20, 1); put16(22, channelCount); put32(24, rate);
        put32(28, rate * channelCount * 2); put16(32, channelCount * 2); put16(34, 16);
        memcpy(buffer + 36, "data", 4); put32(40, data);
    }

    static void *create(void *) { return instance; }
    static void *destroy(void *) { return nullptr; }
    static int control(void *, int, int) { return 0; }
    static int handle(void *priv, void *input, void *)
    {
        PetPcmRecorder *self = (PetPcmRecorder *)priv;
        mm_queue_item_t *item = (mm_queue_item_t *)input;
        if (!self || !item || !self->armed || !item->data_addr) return 0;
        size_t n = item->size, room = self->capacity - self->length;
        if (n > room) n = room;
        n &= ~(size_t)1; // keep whole 16-bit samples
        memcpy(self->buffer + HEADER_BYTES + self->length, (const void *)item->data_addr, n);
        self->length += n;
        if (self->length >= self->capacity) self->armed = false;
        return 0;
    }
};

PetPcmRecorder *PetPcmRecorder::instance = nullptr;
mm_module_t PetPcmRecorder::module = {
    PetPcmRecorder::create, PetPcmRecorder::destroy, PetPcmRecorder::control, PetPcmRecorder::handle,
    nullptr, nullptr, nullptr, nullptr,
    MM_TYPE_NONE, MM_TYPE_ASINK, "PetPCM"
};
