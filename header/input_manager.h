#pragma once
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
#include <iostream>
#include <ctime>
#include <fstream>

class InputManager {
public:
    InputManager();
    ~InputManager();
    void run();

private:
    SDL_Window* window;
    SDL_GLContext glContext;
    int width, height;
    std::unique_ptr<FractalManager> fractalManager;
    std::unique_ptr<ScreenManager> screenManager;
    GLuint textureShaderProgram, colorShaderProgram;
    GLuint vao, vbo;
    glm::mat4 projection;

    int frameCounter;
    bool scalingMode;
    SDL_FPoint scaleStartPos;
    SDL_FPoint originalDimensions;
    GLuint frozenFrame;
    GLuint currentFrame;
    int tempWidth, tempHeight;
    bool running;

    bool handleEvents();
    void handleMouseClick(const SDL_MouseButtonEvent& event);
    void handleTempScaling(const SDL_Event& event);
    void handleScalingMotion(const SDL_Event& event);
    void handleKeyPress(const std::string& event);
    void handleExitScaling(const SDL_Event& event);
    void handleColorRotation();
    void handleSaturation();
    void handleStrengthen();
    void handleWeaken();
    void update();
    void draw();
};