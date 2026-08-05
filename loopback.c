#include <complex.h>
#include <math.h>
#include "loopback.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

struct LoopbackStream {
    ma_device device;
    float* ringBuffer;
    size_t bufferSize;
    size_t writeIndex;
    size_t readIndex;
    ma_bool32 isInitialized;
};

static void CBFxn_Loopback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    LoopbackStream* stream = (LoopbackStream*)pDevice->pUserData;
    if (!stream || !pInput) return;

    const float* samples = (const float*)pInput;
    size_t channels = pDevice->capture.channels;
    size_t totalSamples = frameCount * channels;

    for (size_t i = 0; i < totalSamples; i++) {
        stream->ringBuffer[stream->writeIndex] = samples[i];
        stream->writeIndex = (stream->writeIndex + 1) % stream->bufferSize;
    }

    (void)pOutput;
}

LoopbackStream* Loopback_Init(uint32_t sampleRate, uint32_t channels, size_t bufferSize) {
    LoopbackStream *stream = calloc(sizeof(*stream), 1);
    stream->bufferSize = bufferSize;
    stream->ringBuffer = (float*)calloc(bufferSize, sizeof(float));
    if (!stream->ringBuffer) return MA_FALSE;

    ma_device_config config = ma_device_config_init(ma_device_type_loopback);
    config.capture.format   = ma_format_f32;
    config.capture.channels = channels;
    config.sampleRate       = sampleRate;
    config.dataCallback     = CBFxn_Loopback;
    config.pUserData        = stream;

#if defined(__APPLE__)
    // macOS does not support native OS loopback via ma_device_type_loopback.
    // Fall back to capture mode to pick up virtual devices (BlackHole, Soundflower, Loopback).
    config.deviceType = ma_device_type_capture;
#endif

    if (ma_device_init(NULL, &config, &stream->device) != MA_SUCCESS) {
        free(stream->ringBuffer);
        stream->ringBuffer = NULL;
        return NULL;
    }

    stream->isInitialized = MA_TRUE;
    return stream;
}

bool Loopback_Start(LoopbackStream* stream) {
    if (!stream || !stream->isInitialized) return MA_FALSE;
    return (ma_device_start(&stream->device) == MA_SUCCESS);
}

void Loopback_Stop(LoopbackStream* stream) {
    if (stream && stream->isInitialized) {
        ma_device_stop(&stream->device);
    }
}

size_t Loopback_ReadLatest(LoopbackStream* stream, float complex* outBuffer, size_t count) {
    if (!stream || !stream->isInitialized || !outBuffer || count == 0) return 0;

    size_t available = count > stream->bufferSize ? stream->bufferSize : count;
    size_t head = stream->writeIndex;

    size_t start = (head >= available) ? (head - available) : (stream->bufferSize + head - available);

    for (size_t i = 0; i < available; i++) {
        float hann = 0.5f * (1.0f - cosf((2.0f * M_PI * i)/(available-1)));
        outBuffer[i] = stream->ringBuffer[(start + i) % stream->bufferSize];
        outBuffer[i] *= hann;
    }

    return available;
}

void Loopback_Uninit(LoopbackStream* stream) {
    if (!stream) return;
    if (stream->isInitialized) {
        ma_device_uninit(&stream->device);
        stream->isInitialized = MA_FALSE;
    }
    if (stream->ringBuffer) {
        free(stream->ringBuffer);
        stream->ringBuffer = NULL;
    }
    free(stream);
}