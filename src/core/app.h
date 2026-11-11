#pragma once
#include <memory>
#include "core/window.h"
#include "input/input_manager.h"
#include "render/collision_field.h"
#include "render/fractal_manager.h"
#include "render/overlay_renderer.h"
#include "scene/screen_manager.h"
#include "ui/ui_manager.h"

class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void run();

private:
    Window window;
    std::unique_ptr<FractalManager> fractalManager;
    std::unique_ptr<CollisionField> collisionField;
    std::unique_ptr<ScreenManager> screenManager;
    std::unique_ptr<OverlayRenderer> overlay;
    std::unique_ptr<UiManager> ui;
    std::unique_ptr<InputManager> input;

    int frameCounter;
    int framesUntilFullscreen;
    double elapsedTime;
    float deltaTime;
    Uint64 lastFrameCounter;

    void pumpEvents();
    void updateDeltaTime();
    void update();
    void draw();
};