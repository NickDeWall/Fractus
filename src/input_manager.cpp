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

    int displayIndex = 0;
    SDL_Rect displayBounds;
    SDL_GetDisplayBounds(displayIndex, &displayBounds);
    width = displayBounds.w;
    height = displayBounds.h;
    
    window = SDL_CreateWindow("Fractal Visualizer", displayBounds.x, displayBounds.y, width, height, SDL_WINDOW_OPENGL);

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
    if (glewInit() != GLEW_OK) {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize GLEW");
    }

    if (TTF_Init() == -1) {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize SDL_ttf");
    }

    font = TTF_OpenFont("..\\..\\assets\\fonts\\BebasNeue-Regular.ttf", 24);
    if (!font) {
        TTF_Quit();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Failed to load font");
    }

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
    lastFrameTime = SDL_GetTicks();
    deltaTime = 0.0f;

    debugWidth = 0;
    debugHeight = 0;
    OtherRenders::initDebugTexture(debugTexture);
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
    while (running) {
        updateDeltaTime();
        running = handleEvents();
        update();
        draw();
        frameCounter++;
        SDL_Delay(1000 / Config::FPS);
    }
}

void InputManager::updateDeltaTime() {
    Uint32 currentTime = SDL_GetTicks();
    deltaTime = (currentTime - lastFrameTime) / 1000.0f;
    lastFrameTime = currentTime;
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
    if (keyState[SDL_SCANCODE_D]) {
        handleKeyPress("rotate_clockwise");
    }
    else if (keyState[SDL_SCANCODE_A]) {
        handleKeyPress("rotate_counterclockwise");
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
        screenManager->getSelectedScreen()->setWidth(tempWidth);
        screenManager->getSelectedScreen()->setHeight(tempHeight);
    }
}

void InputManager::handleMouseClick(const SDL_MouseButtonEvent& event) {
    SDL_FPoint pos = { static_cast<float>(event.x), static_cast<float>(event.y) };
    switch (event.button) {
    case SDL_BUTTON_LEFT:
        screenManager->handleSelection(pos);
        break;
    case SDL_BUTTON_MIDDLE: {
        Screen* newScreen = screenManager->createScreen(pos);
        screenManager->handleSelection(pos);
        break;
    }
    case SDL_BUTTON_RIGHT:
        screenManager->handleSelection(pos);
        if (screenManager->getSelectedScreen()) {
            Screen* selected = screenManager->getSelectedScreen();
            auto& screens = const_cast<std::vector<Screen>&>(screenManager->getScreens());
            auto it = std::find_if(screens.begin(), screens.end(),
                [selected](const Screen& s) { return &s == selected; });
            if (it != screens.end()) {
                screens.erase(it);
            }
            screenManager = std::make_unique<ScreenManager>(*screenManager);
        }
        screenManager->setSelectedScreen(nullptr);
        break;
    }
}

void InputManager::handleKeyPress(const std::string& event) {
    Screen* selected = screenManager->getSelectedScreen();
    if (!selected) return;
    if (event == "rotate_clockwise") {
        screenManager->handleRotation(Config::ROTATION_SPEED * deltaTime);
    }
    else if (event == "rotate_counterclockwise") {
        screenManager->handleRotation(-Config::ROTATION_SPEED * deltaTime);
    }
    else if (event == "cycle_hue_up") {
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
    Screen* selected = screenManager->getSelectedScreen();
    if (!selected) return;
    float h = selected->getTrueH();
    float s = selected->getTrueS();
    float v = selected->getTrueV();
    h = fmod(h + (dir * (Config::COLOR_ROTATION_SPEED * deltaTime)), 1.0f);
    selected->setTrueH(h);
    float r, g, b;
    MathUtils::hsvToRgb(h, s, v, r, g, b);
    SDL_Color newColor = { 
        static_cast<Uint8>(std::round(r)), 
        static_cast<Uint8>(std::round(g)), 
        static_cast<Uint8>(std::round(b)), 
        static_cast<Uint8>(std::round(selected->getTrueA()))
    };
    selected->setColor(newColor);
}


void InputManager::handleSaturation(const int& dir) {
    Screen* selected = screenManager->getSelectedScreen();
    if (!selected) return;
    float h = selected->getTrueH();
    float s = selected->getTrueS();
    float v = selected->getTrueV();
    if (dir == 1)
        s = std::min(s + (Config::SATURATION_CYCLE_SPEED * deltaTime), 1.0f);
    else if (dir == -1)
        s = std::max(s - (Config::SATURATION_CYCLE_SPEED * deltaTime), 0.0f);
    selected->setTrueS(s);
    float r, g, b;
    MathUtils::hsvToRgb(h, s, v, r, g, b);
    SDL_Color newColor = { 
        static_cast<Uint8>(std::round(r)), 
        static_cast<Uint8>(std::round(g)), 
        static_cast<Uint8>(std::round(b)), 
        static_cast<Uint8>(std::round(selected->getTrueA()))};
    selected->setColor(newColor);
}

void InputManager::handleAlpha(const int& dir) {
    Screen* selected = screenManager->getSelectedScreen();
    if (!selected) return;
    
    float currentAlpha = selected->getTrueA();
    float change = Config::ALPHA_CHANGE_SPEED * deltaTime * dir;
    float newAlpha = std::clamp(currentAlpha + change, 0.0f, static_cast<float>(Config::MAX_SCREEN_ALPHA));
    
    selected->setTrueA(newAlpha);
    
    SDL_Color color = selected->getColor();
    SDL_Color newColor = { color.r, color.g, color.b, static_cast<Uint8>(std::round(newAlpha)) };
    selected->setColor(newColor);
}

void InputManager::update() {
    if (!scalingMode) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        SDL_FPoint mousePos = { static_cast<float>(x), static_cast<float>(y) };
        screenManager->handleDragging(mousePos);
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
    
    OtherRenders::renderSelectionOutline(screenManager->getSelectedScreen(), scalingMode, tempWidth, tempHeight, colorShaderProgram, projection, vao);

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