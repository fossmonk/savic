#include <stdlib.h>
#include <raylib.h>
#include <math.h>
#include "viz.h"

static float mapf(float val, float l, float r, float ml, float mr) {
    float normval = (val-l)/(r-l);
    return ml + normval*(mr - ml);
}

static float getMeanValue(float *fftBins, int fftSize, float *min, float *max) {
    float avg = 0.0f;
    float minVal = 1E9;
    float maxVal = 1E-9;
    for(int i = 0; i < fftSize; i++) {
        float f = fftBins[i];
        avg += f;
        if(f > maxVal) maxVal = f;
        if(f < minVal) minVal = f;
    }

    if(min) *min = minVal;
    if(max) *max = maxVal;

    return avg/(float)fftSize;
}

static void VizLines(VizParams params) {
    const float numBins = params.size/params.div;
    const float xInc = params.winSize.width/numBins;
    float x = 0.0f;
    float W = params.winSize.width; float H = params.winSize.height;
    float minval, maxval;
    float mean = getMeanValue(params.fftMagBins, numBins, &minval, &maxval);
    for(int i = 0; i < numBins; ++i) {
        float len = mapf(params.fftMagBins[i], minval, maxval, 0, H*0.5f);
        float newMean = mapf(mean, minval, maxval, 0, H*0.5f);
        len = newMean + 0.6*(len - newMean);
        float y1 = H*0.5;
        float y2 = H*0.5 - len;
        float hue = mapf(i, 0, numBins, 0, 360);
        DrawLineEx((Vector2){x, y1}, (Vector2){x, y2}, 1+(W/160.0f), ColorFromHSV(hue, 0.8, 0.9));
        x += xInc;
    }
}

static void VizCircles(VizParams params) {
    const float numBins = params.size/params.div;
    float W = params.winSize.width; float H = params.winSize.height;
    Vector2 c = (Vector2){W/2, H/2};
    float minval, maxval;
    float mean = getMeanValue(params.fftMagBins, numBins, &minval, &maxval);
    for(int i = 0; i < numBins; ++i) {
        float len = mapf(params.fftMagBins[i], minval, maxval, 0, H*0.5f);
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

static void VizSpikes(VizParams params) {
    float W = params.winSize.width; float H = params.winSize.height;
    float len = fminf(W, H);
    float r = 0.05 * len;
    const int numBins = params.size/params.div;
    const float angleInc = 2*PI/numBins;
    const Vector2 c = (Vector2){W/2, H/2};
    float angle = nextAngle(angleInc, 2*PI);
    float minval, maxval;
    float mean = getMeanValue(params.fftMagBins, numBins, &minval, &maxval);
    for(int i = 0; i < numBins; ++i) {
        float spikelen = mapf(params.fftMagBins[i], minval, maxval, 0, len*0.5f);
        float newMean = mapf(mean, minval, maxval, 0, len*0.5f);
        spikelen = newMean*1.5f + 0.6f*(spikelen - newMean);
        Vector2 p = (Vector2){c.x + spikelen * cosf(angle), c.y + spikelen * sinf(angle)};
        float hue = mapf(i, 0, numBins, 0, 360);
        DrawLineEx(c, p, 1+(W/160.0f), ColorFromHSV(hue, 0.8, 0.9));
        angle = fmodf(angle+angleInc, 2*PI);
    }
    DrawCircle(c.x, c.y, r, GetColor(0x111317FF));
}

static void VizLightGrid(VizParams params) {
    const float numBins = params.size/params.div;
    int cols = (int)sqrtf(numBins*params.winSize.width/params.winSize.height);
    int rows = (int)sqrtf(numBins*params.winSize.height/params.winSize.width);
    int cellsz = (int)sqrtf(params.winSize.height*params.winSize.width/numBins);

    float dx = (params.winSize.width - cols*cellsz) * 0.5;
    float dy = (params.winSize.height - rows*cellsz) * 0.5;

    Rectangle newArea = {
        .x = params.winSize.x + dx, .y = params.winSize.y + dy, .width = cols*cellsz, .height = rows*cellsz
    };
    
    int N = cols*rows;

    float minval, maxval;
    float mean = getMeanValue(params.fftMagBins, N, &minval, &maxval);

    for(int i = 0; i < N; ++i) {
        float cx = (i % cols) * cellsz + newArea.x + cellsz*0.5;
        float cy = (i / cols) * cellsz + newArea.y + cellsz*0.5;
        float len = mapf(params.fftMagBins[i], minval, maxval, 0, cellsz*0.5f);
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