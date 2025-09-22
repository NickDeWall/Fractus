#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "screen_manager.h"
#include "fractal_manager.h"

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
    
    bool running = true;
    int frameCounter;
    GLuint currentFrame;
    
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

public:
    InputManager();
    ~InputManager();
    void run();

private:
    bool handleEvents();
    void handleTempScaling(const SDL_Event& event);
    void handleScalingMotion(const SDL_Event& event);
    void handleExitScaling(const SDL_Event& event);
    void handleMouseClick(const SDL_MouseButtonEvent& event);
    void handleKeyPress(const std::string& event);
    void handleColorRotation();
    void handleSaturation();
    void handleStrengthen();
    void handleWeaken();
    void update();
    void drawFPS();
    void draw();
};