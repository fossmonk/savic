#ifndef _VIZ_H_
#define _VIZ_H_
#include <complex.h>
#include <raylib.h>

typedef void (*VizFxn)(float complex *fftBins, int fftSize, int div, Rectangle winSize);

typedef struct {
    const char* name;
    VizFxn fxn;
} Viz;

Viz* Viz_getVTable(void);
int Viz_getCount(void);

#endif