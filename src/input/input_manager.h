#pragma once
#include <SDL2/SDL.h>
#include <vector>

class Window;
class ScreenManager;
class UiManager;
class Ball;

class InputManager {
public:
    InputManager(Window& window, ScreenManager& screenManager, UiManager& ui, std::vector<Ball>& balls);

    void handleEvent(const SDL_Event& event);
    void update(float deltaTime);

    bool quitRequested() const { return quit; }
    bool isStretching() const { return stretching; }
    int getRotateInput() const { return rotateInput; }

private:
    Window& window;
    ScreenManager& screens;
    UiManager& ui;
    std::vector<Ball>& balls;

    bool quit;
    bool stretching;
    int rotateInput;
    int heldBall;

    void handleKeyDown(const SDL_KeyboardEvent& event);
    void handleMouseDown(const SDL_MouseButtonEvent& event);
    void applyHeldKeys(float deltaTime);
    void applyColorRotation(int dir, float deltaTime);
    void applySaturation(int dir, float deltaTime);
    void applyAlpha(int dir, float deltaTime);
};