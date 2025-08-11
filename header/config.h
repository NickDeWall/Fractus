#pragma once
#include <SDL2/SDL.h>

namespace Config {
    constexpr int SCREEN_WIDTH = 1600;
    constexpr int SCREEN_HEIGHT = 950;
    constexpr int FPS = 140;

    constexpr SDL_Color DEFAULT_SCREEN_COLOR = { 66, 135, 245, 15 };
    constexpr float INITIAL_SCREEN_SIZE_RATIO = 0.25f;
    constexpr Uint8 OUTLINE_ALPHA = 40;
    constexpr int OUTLINE_THICKNESS = 5;
    constexpr Uint8 OUTLINE_SCALE_INCREASE = 40;

    constexpr float SCALE_FACTOR_UP = 1.05f;
    constexpr float SCALE_FACTOR_DOWN = 0.95f;
    constexpr int MIN_SCREEN_SIZE = 0;
    constexpr float MAX_SCREEN_RATIO = 1.0f;
    constexpr float ROTATION_SPEED = 1.0f;
    constexpr float COLOR_ROTATION_SPEED = 0.002f;
    constexpr float BRIGHTNESS_CYCLE_SPEED = 0.01f;
    constexpr float ALPHA_CHANGE_SPEED = 0.1f;
    constexpr Uint8 MAX_SCREEN_ALPHA = 100;

    constexpr const char* FRAME_SAVE_DIR = "frames";
    constexpr bool DEV_TOOLS = true;

    constexpr double PI = 3.14159265358979323846;

    constexpr SDL_Color BACKGROUND_COLOR = { 0, 0, 0, 255 };

    constexpr int MAX_CACHED_SURFACES = 20;
    constexpr bool USE_HARDWARE_ACCEL = true;

    constexpr bool SHOW_FPS = true;
}
