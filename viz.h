#ifndef _VIZ_H_
#define _VIZ_H_
#include <complex.h>
#include <raylib.h>

typedef struct {
    Rectangle winSize;
    float *fftMagBins;
    float *rawSamples;
    int size;
    int div;
    float deltaTime;
} VizParams;

typedef void (*VizFxn)(VizParams params);

typedef struct {
    const char* name;
    VizFxn fxn;
} Viz;

Viz* Viz_getVTable(void);
int Viz_getCount(void);

#endif