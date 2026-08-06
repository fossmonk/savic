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

#ifndef __WIN32
// Helper to check if a string contains a substring (case-insensitive)
static bool StringContainsCaseInsensitive(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    
    char h[256] = {0};
    char n[256] = {0};
    
    for (int i = 0; haystack[i] && i < 255; i++) h[i] = (char)tolower((unsigned char)haystack[i]);
    for (int i = 0; needle[i] && i < 255; i++) n[i] = (char)tolower((unsigned char)needle[i]);

    return strstr(h, n) != NULL;
}

static bool FindLoopbackDeviceID(ma_context* pContext, ma_device_id* pOutDeviceID) {
    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;

    if (ma_context_get_devices(pContext, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        return false;
    }

    // Keywords ordered by likelihood across macOS & Linux
    const char* keywords[] = {
        "blackhole",     // macOS popular virtual driver
        "soundflower",   // macOS legacy virtual driver
        "loopback",      // Rogue Amoeba / PipeWire Loopback
        "monitor",       // PulseAudio / PipeWire monitor streams (e.g., "Built-in Audio Analog Stereo Monitor")
        "stereo mix",    // Windows fallback if used in capture mode
        "virtual"        // Generic fallback
    };
    size_t keywordCount = sizeof(keywords) / sizeof(keywords[0]);

    // First pass: look for exact keyword matches in device names
    for (size_t k = 0; k < keywordCount; k++) {
        for (ma_uint32 i = 0; i < captureCount; i++) {
            if (StringContainsCaseInsensitive(pCaptureInfos[i].name, keywords[k])) {
                *pOutDeviceID = pCaptureInfos[i].id;
                printf("[Loopback] Found matching capture device: '%s'\n", pCaptureInfos[i].name);
                return true;
            }
        }
    }

    return false;
}
#endif

LoopbackStream* Loopback_Init(uint32_t sampleRate, uint32_t channels, size_t bufferSize) {
    LoopbackStream *stream = (LoopbackStream*)calloc(1, sizeof(*stream));
    if (!stream) return NULL;

    stream->bufferSize = bufferSize;
    stream->ringBuffer = (float*)calloc(bufferSize, sizeof(float));
    if (!stream->ringBuffer) {
        free(stream);
        return NULL;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_loopback);
    config.capture.format   = ma_format_f32;
    config.capture.channels = channels;
    config.sampleRate       = sampleRate;
    config.dataCallback     = CBFxn_Loopback;
    config.pUserData        = stream;

    ma_context context;
    bool hasCustomDevice = false;
    
#if !defined(_WIN32)
    ma_device_id foundDeviceID;
    // Linux/macOS fallback: switch to capture mode and search for loopback/monitor devices
    config.deviceType = ma_device_type_capture;

    if (ma_context_init(NULL, 0, NULL, &context) == MA_SUCCESS) {
        if (FindLoopbackDeviceID(&context, &foundDeviceID)) {
            config.capture.pDeviceID = &foundDeviceID;
            hasCustomDevice = true;
        } else {
            printf("[Loopback] Warning: No virtual loopback device found by name. Falling back to default capture (Microphone).\n");
        }
    }
#endif

    // Initialize the device (passing context if initialized, else NULL)
    ma_result result = ma_device_init(hasCustomDevice ? &context : NULL, &config, &stream->device);

    if (hasCustomDevice) {
        ma_context_uninit(&context);
    }

    if (result != MA_SUCCESS) {
        free(stream->ringBuffer);
        free(stream);
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