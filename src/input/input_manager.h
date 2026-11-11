#pragma once
#include <SDL2/SDL.h>

class Window;
class ScreenManager;
class UiManager;

class InputManager {
public:
    InputManager(Window& window, ScreenManager& screenManager, UiManager& ui);

    void handleEvent(const SDL_Event& event);
    void update(float deltaTime);

    bool quitRequested() const { return quit; }
    bool isStretching() const { return stretching; }
    int getRotateInput() const { return rotateInput; }

private:
    Window& window;
    ScreenManager& screens;
    UiManager& ui;

    bool quit;
    bool stretching;
    int rotateInput;

    void handleKeyDown(const SDL_KeyboardEvent& event);
    void handleMouseDown(const SDL_MouseButtonEvent& event);
    void applyHeldKeys(float deltaTime);
    void applyColorRotation(int dir, float deltaTime);
    void applySaturation(int dir, float deltaTime);
    void applyAlpha(int dir, float deltaTime);
};