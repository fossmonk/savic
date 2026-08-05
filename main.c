#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include "loopback.h"
#include "viz.h"
#include <raylib.h>

#define FFT_SIZE (512)
#define LOG2FFT_SIZE (9)
#define INIT_W (800)
#define INIT_H (450)
#define BG_COLOR (0x111317FF)

float complex gFFT[FFT_SIZE];
unsigned int gBitReversalTable[FFT_SIZE];

Viz* gViz;
int gVizCount = 0;
int gCurrViz = 0;
bool gWinDecor = true;
bool gWinFullScreen = false;

float gSW = INIT_W;
float gSH = INIT_H;

void initBitReversalTable(void) {
    for (unsigned int num = 0; num < FFT_SIZE; num++) {
        unsigned int reversed = 0;
        for (unsigned int i = 0; i < LOG2FFT_SIZE; i++) {
            if (num & (1 << i)) {
                reversed |= (1 << ((LOG2FFT_SIZE - 1) - i));
            }
        }
        gBitReversalTable[num] = reversed;
    }
}

void fft512(float complex *fftBins) {
    for (unsigned int i = 0; i < FFT_SIZE; i++) {
        unsigned int rev = gBitReversalTable[i];
        if (i < rev) {
            float complex temp = fftBins[i];
            fftBins[i] = fftBins[rev];
            fftBins[rev] = temp;
        }
    }
    for (unsigned int len = 2; len <= FFT_SIZE; len <<= 1) {
        float angle = -2 * PI / len;
        float complex wlen = cosf(angle) + sinf(angle) * I;

        for (unsigned int i = 0; i < FFT_SIZE; i += len) {
            float complex w = 1.0 + 0.0 * I;
            for (unsigned int j = 0; j < len / 2; j++) {
                float complex u = fftBins[i + j];
                float complex v = fftBins[i + j + len / 2] * w;

                fftBins[i + j] = u + v;
                fftBins[i + j + len / 2] = u - v;

                w *= wlen;
            }
        }
    }
}

int main() {

    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(INIT_W, INIT_H, "SAVIZ");
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TOPMOST);
    SetWindowMinSize(300, 300);
    SetTargetFPS(60);
    SetExitKey(KEY_Q);

    initBitReversalTable();

    LoopbackStream *loopback = Loopback_Init(48000, 1, 2048);

    if(!loopback) {
        TraceLog(LOG_ERROR, "Failed to init LB Device.\n");
        CloseWindow();
    }

    Loopback_Start(loopback);

    gViz = Viz_getVTable();
    gVizCount = Viz_getCount();

    while(!WindowShouldClose()) {
        gSW = GetScreenWidth();
        gSH = GetScreenHeight();

        Loopback_ReadLatest(loopback, gFFT, FFT_SIZE);
        fft512(gFFT);

        if(IsKeyPressed(KEY_V)) {
            gCurrViz = (gCurrViz + 1) % gVizCount;
        }

        if(IsKeyPressed(KEY_H)) {
            gWinDecor = !gWinDecor;
        }

        if(IsKeyPressed(KEY_F)) {
            gWinFullScreen = !gWinFullScreen;
        }

        gWinDecor ? SetWindowState(FLAG_WINDOW_UNDECORATED) : ClearWindowState(FLAG_WINDOW_UNDECORATED);
        gWinFullScreen ? SetWindowState(FLAG_FULLSCREEN_MODE) : ClearWindowState(FLAG_FULLSCREEN_MODE);
        
        BeginDrawing();
        ClearBackground(GetColor(BG_COLOR));
        gViz[gCurrViz].fxn(gFFT, FFT_SIZE, 12, (Rectangle){0, 0, gSW, gSH});
        float w = MeasureText(gViz[gCurrViz].name, 20);
        if(IsCursorOnScreen()) DrawText(gViz[gCurrViz].name, (gSW-w)*0.5, 0, 20, GREEN);
        EndDrawing();
    }

    Loopback_Stop(loopback);
    Loopback_Uninit(loopback);
    CloseWindow();
    return 0;
}