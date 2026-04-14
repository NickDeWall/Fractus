#pragma once
#include <SDL2/SDL.h>

namespace Config {
    constexpr int SCREEN_WIDTH = 1600;
    constexpr int SCREEN_HEIGHT = 950;
    constexpr int FPS = 600;

    constexpr SDL_Color DEFAULT_SCREEN_COLOR = { 66, 135, 245, 15 };
    constexpr float INITIAL_SCREEN_SIZE_RATIO = 0.25f;
    constexpr Uint8 OUTLINE_ALPHA = 30;
    constexpr int OUTLINE_THICKNESS = 3;
    constexpr Uint8 OUTLINE_SCALE_INCREASE = 40;

    constexpr float SCALE_FACTOR_UP = 1.08f;
    constexpr float SCALE_FACTOR_DOWN = 0.92f;
    constexpr int MIN_SCREEN_SIZE = 0;
    constexpr float MAX_SCREEN_RATIO = 0.99f;
    constexpr float ROTATION_SPEED = 90.0f;
    constexpr float COLOR_ROTATION_SPEED = 0.2f;
    constexpr float SATURATION_CYCLE_SPEED = 1.4f;
    constexpr float ALPHA_CHANGE_SPEED = 20.0f;
    constexpr Uint8 MAX_SCREEN_ALPHA = 70;

    constexpr const char* FRAME_SAVE_DIR = "frames";
    constexpr bool DEV_TOOLS = true;

    constexpr double PI = 3.14159265358979323846;

    constexpr SDL_Color BACKGROUND_COLOR = { 0, 0, 0, 255 };

    constexpr int MAX_CACHED_SURFACES = 20;
    constexpr bool USE_HARDWARE_ACCEL = true;

    constexpr bool SHOW_FPS = true;

    constexpr int FPS_UPDATE_INTERVAL_MS = 1000;
    constexpr float FPS_WIDTH_RATIO = 0.07f;
    constexpr float FPS_HEIGHT_RATIO = 0.07f;
    constexpr SDL_Color FPS_TEXT_COLOR = { 255, 255, 255, 60 };
}
