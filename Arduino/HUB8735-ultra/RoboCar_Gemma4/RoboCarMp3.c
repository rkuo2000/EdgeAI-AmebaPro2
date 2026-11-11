// Bounded MP3 player (see RoboCarMp3.h). Adapted from the core's helix_mp3_drv.c;
// all symbols are static so nothing collides with that file.
#include "Arduino.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "mp3dec.h"
#include "audio_api.h"
#include "RoboCarMp3.h"

#define RC_PAGE_SIZE   2304                 // bytes per DMA page (1152 16-bit samples)
#define RC_PAGE_NUM    4
#define RC_QUEUE_LEN   10                   // PCM pages waiting for the DMA

static audio_t rcAudio;
static QueueHandle_t rcQueue = NULL;
static uint8_t rcDmaTx[RC_PAGE_SIZE * RC_PAGE_NUM] __attribute__((aligned(0x20)));
static uint8_t rcDmaRx[RC_PAGE_SIZE * RC_PAGE_NUM] __attribute__((aligned(0x20)));
static int16_t rcPage[RC_PAGE_SIZE / 2];
static uint32_t rcPageLen = 0;
static short rcDecode[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];
static volatile uint32_t rcTxIrqs = 0;
static HMP3Decoder rcDecoder = NULL;    // created once: this helix build has no MP3FreeDecoder()

static void rcTxDone(uint32_t arg, uint8_t *pbuf)
{
    (void)arg;
    (void)pbuf;
    BaseType_t woken = pdFALSE;
    uint8_t *p = audio_get_tx_page_adr(&rcAudio);
    if (xQueueReceiveFromISR(rcQueue, p, &woken) != pdPASS) memset(p, 0, RC_PAGE_SIZE);
    audio_set_tx_page(&rcAudio, p);
    rcTxIrqs++;
    portYIELD_FROM_ISR(woken);
}

static void rcRxDone(uint32_t arg, uint8_t *pbuf)
{
    (void)pbuf;
    audio_set_rx_page((audio_t *)arg);
}

static void rcStart(int sampleRate, uint8_t vol)
{
    uint8_t sr = ASR_16KHZ;
    switch (sampleRate) {
        case 8000:  sr = ASR_8KHZ; break;
        case 16000: sr = ASR_16KHZ; break;
        case 24000: sr = ASR_24KHZ; break;
        case 32000: sr = ASR_32KHZ; break;
        case 44100: sr = ASR_44p1KHZ; break;
        case 48000: sr = ASR_48KHZ; break;
        default: break;
    }
    audio_init(&rcAudio, OUTPUT_SINGLE_EDNED, MIC_SINGLE_EDNED, AUDIO_CODEC_2p8V);
    audio_dac_digital_vol(&rcAudio, vol);
    audio_set_dma_buffer(&rcAudio, rcDmaTx, rcDmaRx, RC_PAGE_SIZE, RC_PAGE_NUM);
    audio_rx_irq_handler(&rcAudio, (audio_irq_handler)rcRxDone, (uint32_t *)&rcAudio);
    audio_tx_irq_handler(&rcAudio, (audio_irq_handler)rcTxDone, (uint32_t *)&rcAudio);
    audio_set_param(&rcAudio, sr, WL_16BIT);
    for (int i = 0; i < RC_PAGE_NUM - 1; i++) {    // page count - 1: the RX interrupt comes first
        uint8_t *p = audio_get_tx_page_adr(&rcAudio);
        if (p) {
            memset(p, 0, RC_PAGE_SIZE);
            audio_set_tx_page(&rcAudio, p);
        }
        audio_set_rx_page(&rcAudio);
    }
    audio_trx_start(&rcAudio);
}

// Queue one full page; false if the DMA did not take a page within stallMs.
static int rcSendPage(uint32_t stallMs)
{
    rcPageLen = 0;
    return xQueueSend(rcQueue, rcPage, pdMS_TO_TICKS(stallMs)) == pdPASS;
}

RoboMp3Result roboPlayMp3(const uint8_t *data, uint32_t len, uint8_t vol, uint32_t stallMs)
{
    RoboMp3Result r;
    memset(&r, 0, sizeof(r));
    if (!rcQueue) rcQueue = xQueueCreate(RC_QUEUE_LEN, RC_PAGE_SIZE);
    if (!rcDecoder) rcDecoder = MP3InitDecoder();
    HMP3Decoder dec = rcDecoder;
    if (!rcQueue || !dec) {
        r.noMemory = 1;
        return r;
    }
    xQueueReset(rcQueue);
    rcPageLen = 0;
    rcTxIrqs = 0;
    MP3DecodeSetChannel(0);

    unsigned char *in = (unsigned char *)data;
    int left = (int)len;
    int started = 0;
    while (left > 6 && !r.stalled) {
        int ret = MP3Decode(dec, &in, &left, rcDecode, 0);
        if (ret == ERR_MP3_INDATA_UNDERFLOW) break;
        if (ret) {    // bad frame: resync one byte later
            in++;
            left--;
            continue;
        }
        MP3FrameInfo fi;
        MP3GetLastFrameInfo(dec, &fi);
        int n = fi.outputSamps;
        if (fi.nChans == 2) {    // keep the left channel
            n /= 2;
            for (int i = 0; i < n; i++) rcDecode[i] = rcDecode[2 * i];
        }
        if (!started) {
            rcStart(fi.samprate, vol);
            r.sampleRate = fi.samprate;
            started = 1;
        }
        for (int i = 0; i < n && !r.stalled; i++) {
            rcPage[rcPageLen++] = rcDecode[i];
            if (rcPageLen == RC_PAGE_SIZE / 2 && !rcSendPage(stallMs)) r.stalled = 1;
        }
        r.frames++;
    }

    if (started) {
        if (!r.stalled && rcPageLen) {    // last partial page, padded with silence
            memset(&rcPage[rcPageLen], 0, (RC_PAGE_SIZE / 2 - rcPageLen) * sizeof(int16_t));
            if (!rcSendPage(stallMs)) r.stalled = 1;
        }
        // Wait until the DMA has taken every queued page, but never longer than stallMs
        // without progress, then let the pages already in the DMA ring play out.
        uint32_t lastIrqs = rcTxIrqs, t0 = millis();
        while (!r.stalled && uxQueueMessagesWaiting(rcQueue)) {
            if (rcTxIrqs != lastIrqs) {
                lastIrqs = rcTxIrqs;
                t0 = millis();
            } else if (millis() - t0 > stallMs) {
                r.stalled = 1;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (!r.stalled) vTaskDelay(pdMS_TO_TICKS(300));
        audio_deinit(&rcAudio);    // stops the DMA + interrupts before the queue is reused
    }
    r.txIrqs = rcTxIrqs;
    return r;
}
