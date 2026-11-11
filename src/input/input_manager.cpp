#include "input/input_manager.h"
#include "core/config.h"
#include "core/window.h"
#include "scene/screen.h"
#include "scene/screen_manager.h"
#include "ui/ui_manager.h"
#include <algorithm>

InputManager::InputManager(Window& window, ScreenManager& screenManager, UiManager& ui)
    : window(window), screens(screenManager), ui(ui),
      quit(false), stretching(false), rotateInput(0) {}

void InputManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_QUIT:
        quit = true;
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (!stretching && !ui.wantsMouse()) {
            handleMouseDown(event.button);
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
            screens.endSelection({ static_cast<float>(event.button.x), static_cast<float>(event.button.y) },
                (SDL_GetModState() & KMOD_CTRL) != 0);
        }
        break;
    case SDL_MOUSEWHEEL:
        if (!stretching && !ui.wantsMouse()) {
            screens.handleScaling(event.wheel.y);
        }
        break;
    case SDL_KEYDOWN:
        handleKeyDown(event.key);
        break;
    case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_SPACE && stretching) {
            stretching = false;
            screens.endStretch();
        }
        break;
    }
}

void InputManager::handleKeyDown(const SDL_KeyboardEvent& event) {
    if (event.repeat != 0 || ui.wantsKeyboard()) return;

    const SDL_Keycode sym = event.keysym.sym;
    if (sym == SDLK_ESCAPE) {
        if (!ui.isScreenMenuOpen()) {
            quit = true;
        } else {
            ui.closeScreenMenu();
        }
    }
    else if (sym == SDLK_SPACE && !screens.getSelectedScreens().empty()) {
        int x = 0;
        int y = 0;
        SDL_GetMouseState(&x, &y);
        stretching = true;
        screens.beginStretch({ static_cast<float>(x), static_cast<float>(y) });
    }
    else if (sym == SDLK_e && !screens.getSelectedScreens().empty()) {
        ui.toggleScreenMenu();
    }
    else if (sym == SDLK_RSHIFT || sym == SDLK_F11 ||
        (sym == SDLK_RETURN && (event.keysym.mod & KMOD_ALT))) {
        window.toggleFullscreen();
    }
}

void InputManager::handleMouseDown(const SDL_MouseButtonEvent& event) {
    const SDL_FPoint pos = { static_cast<float>(event.x), static_cast<float>(event.y) };
    const bool additive = (SDL_GetModState() & KMOD_CTRL) != 0;

    switch (event.button) {
    case SDL_BUTTON_LEFT:
        screens.handleSelection(pos, additive);
        break;
    case SDL_BUTTON_MIDDLE:
        screens.createScreen(pos);
        screens.handleSelection(pos, false);
        break;
    case SDL_BUTTON_RIGHT:
        screens.handleDeleteClick(pos);
        break;
    }
}

void InputManager::update(float deltaTime) {
    int x = 0;
    int y = 0;
    SDL_GetMouseState(&x, &y);
    const SDL_FPoint mousePos = { static_cast<float>(x), static_cast<float>(y) };

    if (stretching) {
        screens.updateStretch(mousePos);
    }
    screens.updatePointer(mousePos, !stretching && !ui.wantsMouse());

    applyHeldKeys(deltaTime);
}

void InputManager::applyHeldKeys(float deltaTime) {
    rotateInput = 0;
    if (ui.wantsKeyboard()) return;

    const Uint8* keyState = SDL_GetKeyboardState(nullptr);
    if (keyState[SDL_SCANCODE_D]) {
        rotateInput = 1;
    }
    else if (keyState[SDL_SCANCODE_A]) {
        rotateInput = -1;
    }
    else if (keyState[SDL_SCANCODE_W]) {
        applyAlpha(1, deltaTime);
    }
    else if (keyState[SDL_SCANCODE_S]) {
        applyAlpha(-1, deltaTime);
    }
    else if (Config::DEV_TOOLS) {
        if (keyState[SDL_SCANCODE_UP]) {
            applySaturation(1, deltaTime);
        }
        else if (keyState[SDL_SCANCODE_DOWN]) {
            applySaturation(-1, deltaTime);
        }
        else if (keyState[SDL_SCANCODE_RIGHT]) {
            applyColorRotation(1, deltaTime);
        }
        else if (keyState[SDL_SCANCODE_LEFT]) {
            applyColorRotation(-1, deltaTime);
        }
    }
}

void InputManager::applyColorRotation(int dir, float deltaTime) {
    for (Screen* screen : screens.getSelectedScreens()) {
        screen->setHue(screen->getHue() + dir * Config::COLOR_ROTATION_SPEED * deltaTime);
    }
}

void InputManager::applySaturation(int dir, float deltaTime) {
    for (Screen* screen : screens.getSelectedScreens()) {
        screen->setSaturation(screen->getSaturation() + dir * Config::SATURATION_CYCLE_SPEED * deltaTime);
    }
}

void InputManager::applyAlpha(int dir, float deltaTime) {
    const float cap = Config::MAX_SCREEN_ALPHA / 255.0f;
    for (Screen* screen : screens.getSelectedScreens()) {
        const float current = screen->getAlpha();
        const float next = current + dir * (Config::ALPHA_CHANGE_SPEED / 255.0f) * deltaTime;
        screen->setAlpha(std::min(next, std::max(current, cap)));
    }
}