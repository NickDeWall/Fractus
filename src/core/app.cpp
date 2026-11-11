#include "core/app.h"
#include "core/config.h"
#include <algorithm>

App::App()
    : window("Fractus", Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT),
      frameCounter(0), framesUntilFullscreen(-1), elapsedTime(0.0), deltaTime(0.0f),
      lastFrameCounter(SDL_GetPerformanceCounter()) {
    fractalManager = std::make_unique<FractalManager>(window.getWidth(), window.getHeight(), window.getProjection());
    collisionField = std::make_unique<CollisionField>();
    screenManager = std::make_unique<ScreenManager>(window.getWidth(), window.getHeight());
    overlay = std::make_unique<OverlayRenderer>();
    ui = std::make_unique<UiManager>(window.handle(), window.context());
    input = std::make_unique<InputManager>(window, *screenManager, *ui);

    if (Config::WRITE_LOG_FILE) {
        window.logInfo();
    }

    if (Config::START_FULLSCREEN) {
        framesUntilFullscreen = Config::FULLSCREEN_DELAY_FRAMES;
        if (framesUntilFullscreen <= 0) {
            window.setFullscreen(true);
            framesUntilFullscreen = -1;
        }
    }
}

App::~App() {
    input.reset();
    ui.reset();
    overlay.reset();
    screenManager.reset();
    collisionField.reset();
    fractalManager.reset();
}

void App::run() {
    const Uint64 counterFreq = SDL_GetPerformanceFrequency();
    const Uint64 targetTicks = (Config::FPS > 0) ? (counterFreq / static_cast<Uint64>(Config::FPS)) : 0u;

    while (!input->quitRequested()) {
        const Uint64 frameStart = SDL_GetPerformanceCounter();

        updateDeltaTime();
        pumpEvents();
        if (input->quitRequested()) break;

        if (window.applyPendingResize()) {
            screenManager->resize(window.getWidth(), window.getHeight());
            fractalManager->resize(window.getWidth(), window.getHeight(), window.getProjection());
        }

        if (window.isMinimized()) {
            SDL_Delay(50);
            lastFrameCounter = SDL_GetPerformanceCounter();
            continue;
        }

        update();
        draw();
        frameCounter++;

        if (framesUntilFullscreen > 0 && --framesUntilFullscreen == 0) {
            window.setFullscreen(true);
            framesUntilFullscreen = -1;
        }

        if (targetTicks > 0) {
            const Uint64 oneMs = counterFreq / 1000;
            const Uint64 elapsed = SDL_GetPerformanceCounter() - frameStart;
            if (targetTicks > elapsed + oneMs) {
                const Uint64 sleepTicks = targetTicks - elapsed - oneMs;
                SDL_Delay(static_cast<Uint32>((sleepTicks * 1000) / counterFreq));
            }
            while ((SDL_GetPerformanceCounter() - frameStart) < targetTicks) {
            }
        }
    }
}

void App::pumpEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ui->processEvent(event);
        if (window.handleEvent(event)) continue;
        input->handleEvent(event);
    }
}

void App::updateDeltaTime() {
    const Uint64 currentCounter = SDL_GetPerformanceCounter();
    const Uint64 freq = SDL_GetPerformanceFrequency();
    const float elapsed = static_cast<float>(currentCounter - lastFrameCounter) / static_cast<float>(freq);
    deltaTime = std::min(elapsed, 0.1f);
    lastFrameCounter = currentCounter;
}

void App::update() {
    input->update(deltaTime);
    screenManager->update(deltaTime, input->getRotateInput());
    elapsedTime += deltaTime;
    const GLuint canvas = fractalManager->processFrame(screenManager->getScreens(), frameCounter, elapsedTime);
    if (collisionField->visible) {
        collisionField->update(canvas, window.getWidth(), window.getHeight());
    }
}

void App::draw() {
    glClearColor(Config::BACKGROUND_COLOR.r / 255.0f, Config::BACKGROUND_COLOR.g / 255.0f,
        Config::BACKGROUND_COLOR.b / 255.0f, Config::BACKGROUND_COLOR.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    fractalManager->renderCurrentFrame();

    collisionField->renderOutline();

    for (Screen* screen : screenManager->getSelectedScreens()) {
        fractalManager->renderOutline(*screen, input->isStretching(), window.getProjection());
    }

    if (Config::SHOW_FPS) {
        overlay->drawFps(window.getWidth(), window.getHeight(), window.getProjection());
    }

    if (Config::DEV_TOOLS) {
        overlay->drawDebugText(window.getWidth(), window.getHeight(), window.getProjection());
    }

    ui->render(*screenManager, *collisionField);

    window.swap();
}