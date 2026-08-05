#include <stdlib.h>
#include <raylib.h>
#include <math.h>
#include "viz.h"

static float mapf(float val, float l, float r, float ml, float mr) {
    float normval = (val-l)/(r-l);
    return ml + normval*(mr - ml);
}

static float getMeanValue(float complex *fftBins, int fftSize, float *min, float *max) {
    float avg = 0.0f;
    float minVal = 1E9;
    float maxVal = 1E-9;
    for(int i = 0; i < fftSize; i++) {
        float f = cabsf(fftBins[i]);
        avg += f;
        if(f > maxVal) maxVal = f;
        if(f < minVal) minVal = f;
    }

    if(min) *min = minVal;
    if(max) *max = maxVal;

    return avg/(float)fftSize;
}

static void VizLines(float complex *fftBins, int fftSize, int div, Rectangle drawArea) {
    const float numBins = fftSize/div;
    const float xInc = drawArea.width/numBins;
    float x = 0.0f;
    float H = drawArea.height;
    float minval, maxval;
    float mean = getMeanValue(fftBins, numBins, &minval, &maxval);
    for(int i = 0; i < numBins; ++i) {
        float len = mapf(cabsf(fftBins[i]), minval, maxval, 0, H*0.5f);
        float newMean = mapf(mean, minval, maxval, 0, H*0.5f);
        len = newMean + 0.6*(len - newMean);
        float y1 = H*0.8;
        float y2 = H*0.8 - len;
        float hue = mapf(i, 0, numBins, 0, 360);
        DrawLineEx((Vector2){x, y1}, (Vector2){x, y2}, 1+(drawArea.width/160.0f), ColorFromHSV(hue, 0.8, 0.9));
        x += xInc;
    }
}

static void VizCircles(float complex *fftBins, int fftSize, int div, Rectangle drawArea) {
    const float numBins = fftSize/div;
    float W = drawArea.width; float H = drawArea.height;
    Vector2 c = (Vector2){W/2, H/2};
    float minval, maxval;
    float mean = getMeanValue(fftBins, numBins, &minval, &maxval);
    for(int i = 0; i < numBins; ++i) {
        float len = mapf(cabsf(fftBins[i]), minval, maxval, 0, H*0.5f);
        float newMean = mapf(mean, minval, maxval, 0, H*0.5f);
        len = newMean + 0.6*(len - newMean);
        float hue = mapf(i, 0, numBins, 0, 360);
        DrawCircleLinesV(c, len, ColorFromHSV(hue, 0.8, 0.9));
    }
}

static float nextAngle(float inc, float maxval) {
    static float angle = 0.0f;
    angle = fmodf(angle+inc, maxval);
    return angle;
}

static void VizSpikes(float complex *fftBins, int fftSize, int div, Rectangle drawArea) {
    float W = drawArea.width; float H = drawArea.height;
    float len = fminf(W, H);
    float r = 0.05 * len;
    const int numBins = fftSize/div;
    const float angleInc = 2*PI/numBins;
    const Vector2 c = (Vector2){W/2, H/2};
    float angle = nextAngle(angleInc, 2*PI);
    float minval, maxval;
    float mean = getMeanValue(fftBins, numBins, &minval, &maxval);
    for(int i = 0; i < numBins; ++i) {
        float spikelen = mapf(cabsf(fftBins[i]), minval, maxval, 0, len*0.5f);
        float newMean = mapf(mean, minval, maxval, 0, len*0.5f);
        spikelen = newMean*1.5f + 0.6f*(spikelen - newMean);
        Vector2 p = (Vector2){c.x + spikelen * cosf(angle), c.y + spikelen * sinf(angle)};
        float hue = mapf(i, 0, numBins, 0, 360);
        DrawLineEx(c, p, 1+(W/160.0f), ColorFromHSV(hue, 0.8, 0.9));
        angle = fmodf(angle+angleInc, 2*PI);
    }
    DrawCircle(c.x, c.y, r, GetColor(0x111317FF));
}

static void VizLightGrid(float complex *fftBins, int fftSize, int div, Rectangle drawArea) {
    const float numBins = fftSize/div;
    int cols = (int)sqrtf(numBins*drawArea.width/drawArea.height);
    int rows = (int)sqrtf(numBins*drawArea.height/drawArea.width);
    int cellsz = (int)sqrtf(drawArea.height*drawArea.width/numBins);

    float dx = (drawArea.width - cols*cellsz) * 0.5;
    float dy = (drawArea.height - rows*cellsz) * 0.5;

    Rectangle newArea = {
        .x = drawArea.x + dx, .y = drawArea.y + dy, .width = cols*cellsz, .height = rows*cellsz
    };
    
    int N = cols*rows;

    float minval, maxval;
    float mean = getMeanValue(fftBins, N, &minval, &maxval);

    for(int i = 0; i < N; ++i) {
        float cx = (i % cols) * cellsz + newArea.x + cellsz*0.5;
        float cy = (i / cols) * cellsz + newArea.y + cellsz*0.5;
        float len = mapf(cabsf(fftBins[i]), minval, maxval, 0, cellsz*0.5f);
        float newMean = mapf(mean, minval, maxval, 0, cellsz*0.5f);
        len = newMean + 0.6*(len - newMean);
        float hue = mapf(i, 0, N, 0, 360);
        DrawCircle(cx, cy, len, ColorFromHSV(hue, 0.8, 0.9));
    }
}

static Viz gVTable[] = {
    {"Bar Graph", VizLines},
    {"Concentric Circles", VizCircles},
    {"Spikes", VizSpikes},
    {"Light Grid", VizLightGrid},
};

#define ARRAY_LEN(x) sizeof(x)/sizeof(x[0])

int Viz_getCount(void) {
    return ARRAY_LEN(gVTable);
}

Viz* Viz_getVTable(void) {
    return gVTable;
}