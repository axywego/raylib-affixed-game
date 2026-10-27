#include "config.hpp"

#include "raylib.h"

void cfg::setup() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(cfg::kScreenW, cfg::kScreenH, "ARPG prototype");
    ToggleBorderlessWindowed();
    // SetTargetFPS(GetMonitorRefreshRate(0));
}
