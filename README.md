# SAVIC (System Audio Visualizer In C)

System audio visualizer built with miniaudio and raylib.
Derived from my project [MPIC](https://github.com/fossmonk/mpic). 
Copied the visualizer part of it and added a loopback device to 
capture system audio. Tested in windows.

## Build

```bash
make
```
## Run

Run `savic.exe` or `./savic` based on Windows/POSIX. 

## Controls

- `h/H` - Show or hide window decorations
- `f/F` - Fullscreen mode ON/OFF
- `v/V` - Cycle through visualizations

Visualization name will be printed on top if the mouse is inside the window.