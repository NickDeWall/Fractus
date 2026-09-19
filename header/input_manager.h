#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "screen_manager.h"
#include "fractal_manager.h"
#include "ui_manager.h"

class InputManager {
private:
    SDL_Window* window;
    SDL_GLContext glContext;
    int width, height;
    GLuint textureShaderProgram, colorShaderProgram;
    GLuint vao, vbo;
    glm::mat4 projection;
    
    std::unique_ptr<FractalManager> fractalManager;
    std::unique_ptr<ScreenManager> screenManager;
    std::unique_ptr<UiManager> ui;
    
    bool running = true;
    bool minimized = false;
    bool pendingResize = false;
    bool borderlessFullscreen = false;
    int framesUntilFullscreen = -1;
    SDL_Rect windowedRect = { 0, 0, 0, 0 };
    int frameCounter;
    GLuint currentFrame;
    int rotateInput = 0;
    double elapsedTime = 0.0;
    
    // Scaling mode variables
    bool scalingMode;
    SDL_FPoint scaleStartPos;
    SDL_FPoint originalDimensions;
    GLuint frozenFrame;
    int tempWidth, tempHeight;
    
    // FPS counter variables
    TTF_Font* font;
    GLuint fpsTexture;
    int fpsWidth, fpsHeight;
    Uint32 lastFPSTime;
    int fpsFrameCount;
    Uint64 lastFrameCounter;
    float deltaTime;

    // Debug variables
    GLuint debugTexture;
    int debugWidth, debugHeight;
    std::string currentDebugText;

public:
    InputManager();
    ~InputManager();
    void run();
    void updateDeltaTime();

private:
    bool handleEvents();
    void toggleFullscreen();
    void setFullscreen(bool enable);
    bool isFullscreen() const;
    void handleResize();
    void logStartupInfo();
    void handleScalingMotion(const SDL_Event& event);
    void handleExitScaling(const SDL_Event& event);
    void handleMouseClick(const SDL_MouseButtonEvent& event);
    void handleKeyPress(const std::string& event);
    void handleColorRotation(const int& dir);
    void handleSaturation(const int& dir);
    void handleAlpha(const int& dir);
    void update();
    void drawFPS();
    void draw();
    void setDebugText(const std::string& text);
};