#ifndef _LOOPBACK_H_
#define _LOOPBACK_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct LoopbackStream LoopbackStream;

LoopbackStream* Loopback_Init(uint32_t sampleRate, uint32_t channels, size_t bufferSize);
bool Loopback_Start(LoopbackStream* stream);
void Loopback_Stop(LoopbackStream* stream);
size_t Loopback_ReadLatest(LoopbackStream* stream, float complex* outBuffer, size_t count);
void Loopback_Uninit(LoopbackStream* stream);

#endif //_LOOPBACK_H_