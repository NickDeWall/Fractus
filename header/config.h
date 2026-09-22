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
    constexpr float OUTLINE_MIN_VALUE = 0.6f;

    constexpr float SCALE_FACTOR_UP = 1.08f;
    constexpr float SCALE_FACTOR_DOWN = 0.92f;
    constexpr float SCALE_DURATION = 0.12f;
    constexpr float SCALE_EASE_STRENGTH = 3.0f;
    constexpr float MOVE_SMOOTH_TIME = 0.025f;
    constexpr float DEFAULT_UPDATE_RATE = 140.0f;
    constexpr float MIN_UPDATE_RATE = 0.5f;
    constexpr float MAX_UPDATE_RATE = 500.0f;
    constexpr float MAX_DELAY = 1.0f;
    constexpr float CAPTURE_RATE_NO_DELAY = 140.0f;
    constexpr float CAPTURE_RATE_MAX_DELAY = 40.0f;
    constexpr int HISTORY_SIZE_STEPS = 16;
    constexpr float LOG_POLAR_MIN_RADIUS = 0.01f;
    constexpr float LOG_POLAR_MIN_RADIUS_LOW = 0.0001f;
    constexpr float LOG_POLAR_MIN_RADIUS_HIGH = 0.9f;
    constexpr float JULIA_VIEW_HEIGHT = 3.0f;
    constexpr float JULIA_DEFAULT_REAL = -0.8f;
    constexpr float JULIA_DEFAULT_IMAG = 0.156f;
    constexpr float JULIA_C_LIMIT = 2.0f;
    constexpr float DROSTE_DEFAULT_ZOOM = 3.0f;
    constexpr float DROSTE_MIN_ZOOM = 1.1f;
    constexpr float DROSTE_MAX_ZOOM = 100.0f;
    constexpr int DROSTE_DEFAULT_ARMS = 1;
    constexpr int DROSTE_ARM_LIMIT = 6;
    constexpr int MIN_SCREEN_SIZE = 0;
    constexpr float MAX_SCREEN_RATIO = 0.99f;
    constexpr float ROTATION_SPEED = 140.0f;
    constexpr float ROTATION_RAMP_TIME = 0.3f;
    constexpr float ROTATION_DECEL_TIME = 0.1f;
    constexpr float COLOR_ROTATION_SPEED = 0.2f;
    constexpr float SATURATION_CYCLE_SPEED = 1.4f;
    constexpr float ALPHA_CHANGE_SPEED = 20.0f;
    constexpr Uint8 MAX_SCREEN_ALPHA = 70;

    constexpr const char* FRAME_SAVE_DIR = "frames";
    constexpr bool DEV_TOOLS = true;

    constexpr double PI = 3.14159265358979323846;

    constexpr SDL_Color BACKGROUND_COLOR = { 0, 0, 0, 255 };

    constexpr bool START_FULLSCREEN = true;

    constexpr int FULLSCREEN_DELAY_FRAMES = 3;

    constexpr int FULLSCREEN_INSET = 0;

    constexpr bool VSYNC = false;

    constexpr bool WRITE_LOG_FILE = false;
    constexpr const char* LOG_FILE = "fractal_visualizer_log.txt";

    constexpr float RENDER_SCALE = 1.0f;

    constexpr int MAX_RENDER_WIDTH = 2560;
    constexpr int MAX_RENDER_HEIGHT = 1440;

    constexpr int MAX_CACHED_SURFACES = 20;
    constexpr bool USE_HARDWARE_ACCEL = true;

    constexpr bool SHOW_FPS = true;

    constexpr int FPS_UPDATE_INTERVAL_MS = 1000;
    constexpr float FPS_WIDTH_RATIO = 0.07f;
    constexpr float FPS_HEIGHT_RATIO = 0.07f;
    constexpr SDL_Color FPS_TEXT_COLOR = { 255, 255, 255, 60 };
}