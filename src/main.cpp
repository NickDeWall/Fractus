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
#include <iostream>
#include <ctime>
#include <fstream>

class FractalVisualizer {
public:
    FractalVisualizer() {
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
        
        window = SDL_CreateWindow("Fractal Visualizer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
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
    }

    ~FractalVisualizer() {
        if (frozenFrame) {
            glDeleteTextures(1, &frozenFrame);
        }
        glDeleteProgram(textureShaderProgram);
        glDeleteProgram(colorShaderProgram);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        fractalManager.reset();
        screenManager.reset();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void run() {
        running = true;
        while (running) {
            running = handleEvents();
            update();
            draw();
            frameCounter++;
            SDL_Delay(1000 / Config::FPS);
        }
    }

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

    bool handleEvents() {
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
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_SPACE && screenManager->getSelectedScreen()) {
                    scalingMode = true;
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    scaleStartPos = { static_cast<float>(x), static_cast<float>(y) };
                    originalDimensions = {
                        static_cast<float>(screenManager->getSelectedScreen()->getWidth()),
                        static_cast<float>(screenManager->getSelectedScreen()->getHeight())
                    };
                    tempWidth = static_cast<int>(originalDimensions.x);
                    tempHeight = static_cast<int>(originalDimensions.y);
                }
                break;
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_SPACE && scalingMode) {
                    scalingMode = false;
                    screenManager->getSelectedScreen()->setWidth(tempWidth);
                    screenManager->getSelectedScreen()->setHeight(tempHeight);
                }
                break;
            case SDL_MOUSEMOTION:
                if (scalingMode) {
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    SDL_FPoint currentPos = { static_cast<float>(x), static_cast<float>(y) };
                    float deltaX = currentPos.x - scaleStartPos.x;
                    float deltaY = currentPos.y - scaleStartPos.y;
                    tempWidth = std::max(Config::MIN_SCREEN_SIZE,
                        std::min(static_cast<int>(originalDimensions.x + deltaX),
                            static_cast<int>(width * Config::MAX_SCREEN_RATIO)));
                    tempHeight = std::max(Config::MIN_SCREEN_SIZE,
                        std::min(static_cast<int>(originalDimensions.y - deltaY),
                            static_cast<int>(height * Config::MAX_SCREEN_RATIO)));
                }
                break;
            }
        }

        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        if (keyState[SDL_SCANCODE_ESCAPE]) {
            return false;
        }
        if (keyState[SDL_SCANCODE_D]) {
            handleKeyPress("turn_right");
        }
        else if (keyState[SDL_SCANCODE_A]) {
            handleKeyPress("turn_left");
        }
        else if (keyState[SDL_SCANCODE_W]) {
            handleKeyPress("strengthen");
        }
        else if (keyState[SDL_SCANCODE_S]) {
            handleKeyPress("weaken");
        }
        else if (Config::DEV_TOOLS) {
            if (keyState[SDL_SCANCODE_UP]) {
                handleKeyPress("cycle_hue");
            }
            else if (keyState[SDL_SCANCODE_DOWN]) {
                handleKeyPress("cycle_saturation");
            }
        }
        return true;
    }

    void handleMouseClick(const SDL_MouseButtonEvent& event) {
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
            break;
        }
    }

    void handleKeyPress(const std::string& event) {
        Screen* selected = screenManager->getSelectedScreen();
        if (!selected) return;
        if (event == "turn_right") {
            screenManager->handleRotation(-Config::ROTATION_SPEED);
        }
        else if (event == "turn_left") {
            screenManager->handleRotation(Config::ROTATION_SPEED);
        }
        else if (event == "cycle_hue") {
            handleColorRotation();
        }
        else if (event == "cycle_saturation") {
            handleBrightening();
        }
        else if (event == "strengthen") {
            handleStrengthen();
        }
        else if (event == "weaken") {
            handleWeaken();
        }
    }

    void handleColorRotation() {
        Screen* selected = screenManager->getSelectedScreen();
        if (!selected) return;
        SDL_Color color = selected->getColor();
        float h, s, v;
        MathUtils::rgbToHsv(color.r, color.g, color.b, h, s, v);
        h = fmod(h + Config::COLOR_ROTATION_SPEED, 1.0f);
        Uint8 r, g, b;
        MathUtils::hsvToRgb(h, s, v, r, g, b);
        SDL_Color newColor = { r, g, b, color.a };
        selected->setColor(newColor);
    }

    void handleBrightening() {
        Screen* selected = screenManager->getSelectedScreen();
        if (!selected) return;
        SDL_Color color = selected->getColor();
        float h, s, v;
        MathUtils::rgbToHsv(color.r, color.g, color.b, h, s, v);
        s = fmod(s - Config::BRIGHTNESS_CYCLE_SPEED + 1.0f, 1.0f);
        Uint8 r, g, b;
        MathUtils::hsvToRgb(h, s, v, r, g, b);
        SDL_Color newColor = { r, g, b, color.a };
        selected->setColor(newColor);
    }

    void handleStrengthen() {
        Screen* selected = screenManager->getSelectedScreen();
        if (!selected) return;
        SDL_Color color = selected->getColor();
        Uint8 newAlpha = std::min(static_cast<int>(Config::MAX_SCREEN_ALPHA),
            static_cast<int>(color.a + Config::ALPHA_CHANGE_SPEED * 255));
        SDL_Color newColor = { color.r, color.g, color.b, newAlpha };
        selected->setColor(newColor);
    }

    void handleWeaken() {
        Screen* selected = screenManager->getSelectedScreen();
        if (!selected) return;
        SDL_Color color = selected->getColor();
        Uint8 newAlpha = std::max(0, static_cast<int>(color.a - Config::ALPHA_CHANGE_SPEED * 255));
        SDL_Color newColor = { color.r, color.g, color.b, newAlpha };
        selected->setColor(newColor);
    }

    void update() {
        if (!scalingMode) {
            int x, y;
            SDL_GetMouseState(&x, &y);
            SDL_FPoint mousePos = { static_cast<float>(x), static_cast<float>(y) };
            screenManager->handleDragging(mousePos);
            currentFrame = fractalManager->processFrame(screenManager->getScreens(), frameCounter);
        }
    }

    void draw() {
        glClearColor(Config::BACKGROUND_COLOR.r / 255.0f, Config::BACKGROUND_COLOR.g / 255.0f, Config::BACKGROUND_COLOR.b / 255.0f, Config::BACKGROUND_COLOR.a / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        fractalManager->renderCurrentFrame();
        
        OtherRenders::drawSelectionOutline(screenManager->getSelectedScreen(), scalingMode, tempWidth, tempHeight, colorShaderProgram, projection, vao);
        
        SDL_GL_SwapWindow(window);
    }
};

int main(int argc, char* argv[]) {
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::ctime(&now);
    timestamp.pop_back();

    std::ofstream logFile("fractal_visualizer_log.txt", std::ios::trunc);
    if (!logFile) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open log file");
        return 1;
    }

    std::streambuf* coutBuf = std::cout.rdbuf();
    std::streambuf* cerrBuf = std::cerr.rdbuf();
    std::cout.rdbuf(logFile.rdbuf());
    std::cerr.rdbuf(logFile.rdbuf());

    std::cout << "Fractal Visualizer Started - " << timestamp << std::endl;
    
    try {
        FractalVisualizer app;
        app.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cout << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}