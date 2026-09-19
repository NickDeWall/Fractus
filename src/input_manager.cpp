#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include "config.h" 
#include "screen.h"
#include "screen_manager.h"
#include "fractal_manager.h"
#include "math_utils.h"
#include "shader_manager.h"
#include "input_manager.h"
#include "font_data.h"
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <ctime>
#include <fstream>
#include <string>

InputManager::InputManager() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("Fractus",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window) {
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }

    SDL_GL_GetDrawableSize(window, &width, &height);

    if (Config::VSYNC) {
        if (SDL_GL_SetSwapInterval(-1) != 0) {
            SDL_GL_SetSwapInterval(1);
        }
    } else {
        SDL_GL_SetSwapInterval(0);
    }

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize GLEW");
    }
    glGetError();

    if (TTF_Init() == -1) {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize SDL_ttf");
    }

    font = TTF_OpenFontRW(SDL_RWFromConstMem(BebasNeue_Regular_ttf, static_cast<int>(BebasNeue_Regular_ttf_len)), 1, 24);

    fpsWidth = Config::FPS_WIDTH_RATIO * width;
    fpsHeight = Config::FPS_WIDTH_RATIO * height;
    OtherRenders::initFPSTexture(fpsTexture);

    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 pos;
        layout(location = 1) in vec2 texCoord;
        uniform mat4 projection;
        uniform mat4 model;
        out vec2 vTexCoord;
        void main() {
            gl_Position = projection * model * vec4(pos, 0.0, 1.0);
            vTexCoord = texCoord;
        }
    )";
    const char* textureFragmentShaderSrc = R"(
        #version 330 core
        in vec2 vTexCoord;
        uniform sampler2D tex;
        uniform vec4 color;
        out vec4 fragColor;
        void main() {
            vec4 texColor = texture(tex, vTexCoord);
            fragColor = texColor * color;
        }
    )";
    const char* colorFragmentShaderSrc = R"(
        #version 330 core
        uniform vec4 color;
        out vec4 fragColor;
        void main() {
            fragColor = color;
        }
    )";
    
    textureShaderProgram = ShaderManager::createShaderProgram(vertexShaderSrc, textureFragmentShaderSrc);
    colorShaderProgram = ShaderManager::createShaderProgram(vertexShaderSrc, colorFragmentShaderSrc);
    projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f 
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    fractalManager = std::make_unique<FractalManager>(width, height, textureShaderProgram, colorShaderProgram, projection);
    screenManager = std::make_unique<ScreenManager>(width, height);
    frameCounter = 0;
    scalingMode = false;
    scaleStartPos = { 0, 0 };
    originalDimensions = { 0, 0 };
    frozenFrame = 0;
    tempWidth = 0;
    tempHeight = 0;

    lastFPSTime = SDL_GetTicks();
    fpsFrameCount = 0;
    lastFrameCounter = SDL_GetPerformanceCounter();
    deltaTime = 0.0f;

    debugWidth = 0;
    debugHeight = 0;
    OtherRenders::initDebugTexture(debugTexture);

    if (Config::WRITE_LOG_FILE) {
        logStartupInfo();
    }

    framesUntilFullscreen = Config::START_FULLSCREEN ? Config::FULLSCREEN_DELAY_FRAMES : -1;
    if (framesUntilFullscreen == 0) {
        setFullscreen(true);
        framesUntilFullscreen = -1;
    }
}

InputManager::~InputManager() {
    if (frozenFrame) {
        glDeleteTextures(1, &frozenFrame);
    }
    if (fpsTexture) {
        glDeleteTextures(1, &fpsTexture);
    }
    glDeleteProgram(textureShaderProgram);
    glDeleteProgram(colorShaderProgram);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
    fractalManager.reset();
    screenManager.reset();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (debugTexture) {
        glDeleteTextures(1, &debugTexture);
}
}

void InputManager::run() {
    running = true;

    const Uint64 counterFreq = SDL_GetPerformanceFrequency();
    const Uint64 targetTicks = (Config::FPS > 0) ? (counterFreq / static_cast<Uint64>(Config::FPS)) : 0u;

    while (running) {
        const Uint64 frameStart = SDL_GetPerformanceCounter();

        updateDeltaTime();
        running = handleEvents();
        if (!running) break;

        if (pendingResize) {
            pendingResize = false;
            handleResize();
        }

        if (minimized) {
            SDL_Delay(50);
            lastFrameCounter = SDL_GetPerformanceCounter();
            continue;
        }

        update();
        draw();
        frameCounter++;

        if (framesUntilFullscreen > 0 && --framesUntilFullscreen == 0) {
            setFullscreen(true);
            framesUntilFullscreen = -1;
        }

        if (targetTicks > 0) {
            const Uint64 oneMs = counterFreq / 1000;
            Uint64 elapsed = SDL_GetPerformanceCounter() - frameStart;
            if (targetTicks > elapsed + oneMs) {
                const Uint64 sleepTicks = targetTicks - elapsed - oneMs;
                SDL_Delay(static_cast<Uint32>((sleepTicks * 1000) / counterFreq));
            }
            while ((SDL_GetPerformanceCounter() - frameStart) < targetTicks) {
                // spin
            }
        }
    }
}

void InputManager::updateDeltaTime() {
    const Uint64 currentCounter = SDL_GetPerformanceCounter();
    const Uint64 freq = SDL_GetPerformanceFrequency();
    const float elapsed = static_cast<float>(currentCounter - lastFrameCounter) / static_cast<float>(freq);
    deltaTime = std::min(elapsed, 0.1f);
    lastFrameCounter = currentCounter;
}

bool InputManager::isFullscreen() const {
    return borderlessFullscreen;
}

void InputManager::setFullscreen(bool enable) {
    if (enable == borderlessFullscreen) return;

    if (enable) {
        SDL_GetWindowPosition(window, &windowedRect.x, &windowedRect.y);
        SDL_GetWindowSize(window, &windowedRect.w, &windowedRect.h);

        SDL_Rect bounds;
        const int displayIndex = SDL_GetWindowDisplayIndex(window);
        if (SDL_GetDisplayBounds(displayIndex < 0 ? 0 : displayIndex, &bounds) != 0) {
            setDebugText(std::string("Display bounds failed: ") + SDL_GetError());
            return;
        }

        const int inset = Config::FULLSCREEN_INSET;
        SDL_SetWindowBordered(window, SDL_FALSE);
        SDL_SetWindowPosition(window, bounds.x + inset, bounds.y + inset);
        SDL_SetWindowSize(window, bounds.w - 2 * inset, bounds.h - 2 * inset);
        borderlessFullscreen = true;
    } else {
        SDL_SetWindowBordered(window, SDL_TRUE);
        if (windowedRect.w > 0 && windowedRect.h > 0) {
            SDL_SetWindowSize(window, windowedRect.w, windowedRect.h);
            SDL_SetWindowPosition(window, windowedRect.x, windowedRect.y);
        }
        borderlessFullscreen = false;
    }
}

void InputManager::toggleFullscreen() {
    setFullscreen(!borderlessFullscreen);
}

void InputManager::logStartupInfo() {
    std::ofstream log(Config::LOG_FILE, std::ios::trunc);
    if (!log) return;

    const auto glString = [](GLenum name) -> std::string {
        const GLubyte* value = glGetString(name);
        return value ? reinterpret_cast<const char*>(value) : "(unavailable)";
    };

    log << "VENDOR:        " << glString(GL_VENDOR) << "\n"
        << "RENDERER:      " << glString(GL_RENDERER) << "\n"
        << "GL_VERSION:    " << glString(GL_VERSION) << "\n"
        << "GLSL_VERSION:  " << glString(GL_SHADING_LANGUAGE_VERSION) << "\n"
        << "swap interval: " << SDL_GL_GetSwapInterval() << "\n"
        << "drawable:      " << width << "x" << height << "\n";
}


void InputManager::handleResize() {
    int newWidth = 0, newHeight = 0;
    SDL_GL_GetDrawableSize(window, &newWidth, &newHeight);

    if (newWidth <= 0 || newHeight <= 0) return;
    if (newWidth == width && newHeight == height) return;

    screenManager->resize(newWidth, newHeight);

    width = newWidth;
    height = newHeight;

    glViewport(0, 0, width, height);
    projection = glm::ortho(0.0f, static_cast<float>(width),
                            static_cast<float>(height), 0.0f, -1.0f, 1.0f);

    fractalManager->resize(width, height, projection);
}

bool InputManager::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return false;
        case SDL_MOUSEBUTTONDOWN:
            if (!scalingMode) {
                handleMouseClick(event.button);
            }
            break;
        case SDL_MOUSEWHEEL:
            if (!scalingMode) {
                screenManager->handleScaling(event.wheel.y);
            }
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                pendingResize = true;
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                minimized = true;
                break;
            case SDL_WINDOWEVENT_RESTORED:
            case SDL_WINDOWEVENT_SHOWN:
                minimized = false;
                break;
            }
            break;
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {
                const SDL_Keycode sym = event.key.keysym.sym;
                if (sym == SDLK_RSHIFT || sym == SDLK_F11 ||
                    (sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))) {
                    toggleFullscreen();
                }
            }
            break;
        case SDL_KEYUP:
            handleExitScaling(event);
            break;
        case SDL_MOUSEMOTION:
            handleScalingMotion(event);
            break;
        }
    }

    const Uint8* keyState = SDL_GetKeyboardState(nullptr);
    if (keyState[SDL_SCANCODE_ESCAPE]) {
        return false;
    }
    rotateInput = 0;
    if (keyState[SDL_SCANCODE_D]) {
        rotateInput = 1;
    }
    else if (keyState[SDL_SCANCODE_A]) {
        rotateInput = -1;
    }
    else if (keyState[SDL_SCANCODE_W]) {
        handleKeyPress("strengthen");
    }
    else if (keyState[SDL_SCANCODE_S]) {
        handleKeyPress("weaken");
    }
    else if (Config::DEV_TOOLS) {
        if (keyState[SDL_SCANCODE_UP]) {
            handleKeyPress("cycle_saturation_up");
        }
        else if (keyState[SDL_SCANCODE_DOWN]) {
            handleKeyPress("cycle_saturation_down");
        }
        else if (keyState[SDL_SCANCODE_RIGHT]) {
            handleKeyPress("cycle_hue_up");
        }
        else if (keyState[SDL_SCANCODE_LEFT]) {
            handleKeyPress("cycle_hue_down");
        }
    }
    return true;
}

void InputManager::handleScalingMotion(const SDL_Event& event) {
    if (scalingMode) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        SDL_FPoint currentPos = { static_cast<float>(x), static_cast<float>(y) };
        float deltaX = currentPos.x - scaleStartPos.x;
        float deltaY = currentPos.y - scaleStartPos.y;
        tempWidth = std::max(Config::MIN_SCREEN_SIZE, std::min(static_cast<int>(originalDimensions.x + deltaX), static_cast<int>(width * Config::MAX_SCREEN_RATIO)));
        tempHeight = std::max(Config::MIN_SCREEN_SIZE, std::min(static_cast<int>(originalDimensions.y - deltaY), static_cast<int>(height * Config::MAX_SCREEN_RATIO)));
    }
}

void InputManager::handleExitScaling(const SDL_Event& event) {
    if (event.key.keysym.sym == SDLK_SPACE && scalingMode) {
        scalingMode = false;
        for (Screen* screen : screenManager->getSelectedScreens()) {
            screen->setWidth(tempWidth);
            screen->setHeight(tempHeight);
        }
    }
}

void InputManager::handleMouseClick(const SDL_MouseButtonEvent& event) {
    SDL_FPoint pos = { static_cast<float>(event.x), static_cast<float>(event.y) };
    switch (event.button) {
    case SDL_BUTTON_LEFT:
        screenManager->handleSelection(pos);
        break;
    case SDL_BUTTON_MIDDLE:
        screenManager->createScreen(pos);
        screenManager->handleSelection(pos);
        break;
    case SDL_BUTTON_RIGHT:
        screenManager->handleSelection(pos);
        screenManager->deleteSelected();
        break;
    }
}

void InputManager::handleKeyPress(const std::string& event) {
    if (event == "cycle_hue_up") {
        handleColorRotation(1);
    }
    else if (event == "cycle_hue_down") {
        handleColorRotation(-1);
    }
    else if (event == "cycle_saturation_up") {
        handleSaturation(1);
    }
    else if (event == "cycle_saturation_down") {
        handleSaturation(-1);
    }
    else if (event == "strengthen") {
        handleAlpha(1);
    }
    else if (event == "weaken") {
        handleAlpha(-1);
    }
}

void InputManager::handleColorRotation(const int& dir) {
    for (Screen* screen : screenManager->getSelectedScreens()) {
        screen->setHue(screen->getHue() + dir * Config::COLOR_ROTATION_SPEED * deltaTime);
    }
}

void InputManager::handleSaturation(const int& dir) {
    for (Screen* screen : screenManager->getSelectedScreens()) {
        screen->setSaturation(screen->getSaturation() + dir * Config::SATURATION_CYCLE_SPEED * deltaTime);
    }
}

void InputManager::handleAlpha(const int& dir) {
    const float cap = Config::MAX_SCREEN_ALPHA / 255.0f;
    for (Screen* screen : screenManager->getSelectedScreens()) {
        const float current = screen->getAlpha();
        const float next = current + dir * (Config::ALPHA_CHANGE_SPEED / 255.0f) * deltaTime;
        screen->setAlpha(std::min(next, std::max(current, cap)));
    }
}

void InputManager::update() {
    if (!scalingMode) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        SDL_FPoint mousePos = { static_cast<float>(x), static_cast<float>(y) };
        screenManager->handleDragging(mousePos);
        screenManager->update(deltaTime, rotateInput);
        currentFrame = fractalManager->processFrame(screenManager->getScreens(), frameCounter);
    }
}

void InputManager::drawFPS() {
    Uint32 currentTime = SDL_GetTicks();
    fpsFrameCount++;
    float currentFPS = MathUtils::calculateFPS();

    if (currentTime - lastFPSTime >= Config::FPS_UPDATE_INTERVAL_MS) {
        OtherRenders::updateFPSTexture(static_cast<int>(currentFPS), font, fpsTexture, fpsWidth, fpsHeight);
        lastFPSTime = currentTime;
        fpsFrameCount = 0;
    }

    OtherRenders::renderFPS(width, height, fpsTexture, fpsWidth, fpsHeight, textureShaderProgram, projection, vao);
}

void InputManager::draw() {
    glClearColor(Config::BACKGROUND_COLOR.r / 255.0f, Config::BACKGROUND_COLOR.g / 255.0f, Config::BACKGROUND_COLOR.b / 255.0f, Config::BACKGROUND_COLOR.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    fractalManager->renderCurrentFrame();
    
    for (Screen* screen : screenManager->getSelectedScreens()) {
        OtherRenders::renderSelectionOutline(screen, scalingMode, tempWidth, tempHeight, colorShaderProgram, projection, vao);
    }

    if (Config::SHOW_FPS) {
        drawFPS();
    }

    if (Config::DEV_TOOLS) {
        OtherRenders::renderDebugText(width, height, debugTexture, debugWidth, debugHeight, textureShaderProgram, projection, vao);
    }

    SDL_GL_SwapWindow(window);
}

void InputManager::setDebugText(const std::string& text) {
    currentDebugText = text;
    OtherRenders::updateDebugTexture(currentDebugText, font, debugTexture, debugWidth, debugHeight);
}