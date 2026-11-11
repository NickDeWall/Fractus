#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <unordered_map>
#include "render/display_modes.h"

class Screen;
class ScreenManager;
class CollisionField;
class Ball;

class UiManager {
public:
    UiManager(SDL_Window* window, SDL_GLContext context);
    ~UiManager();

    void processEvent(const SDL_Event& event);
    bool wantsMouse() const;
    bool wantsKeyboard() const;

    void toggleScreenMenu();
    void closeScreenMenu();
    bool isScreenMenuOpen() const;

    void render(ScreenManager& screenManager, CollisionField& collision, const std::vector<Ball>& balls);

private:
    bool screenMenuOpen = false;

    bool sizeEditing = false;
    float sizeBasePercent = 0.0f;
    std::unordered_map<int, SDL_FPoint> sizeBaseline;

    bool rotationEditing = false;
    float rotationBaseAngle = 0.0f;
    std::unordered_map<int, float> rotationBaseline;
    std::unordered_map<int, SDL_FPoint> rotationOrigins;
    SDL_FPoint rotationCenter = { 0.0f, 0.0f };

    void drawScreenMenu(ScreenManager& screenManager, const std::vector<Screen*>& selected, CollisionField& collision);
    void drawSizeSlider(ScreenManager& screenManager, const std::vector<Screen*>& selected);
    void drawProportions(ScreenManager& screenManager, const std::vector<Screen*>& selected);
    void drawRotationControl(ScreenManager& screenManager, const std::vector<Screen*>& selected);
    void drawRateSlider(const std::vector<Screen*>& selected);
    void drawDelaySlider(const std::vector<Screen*>& selected);
    void drawModeBrowser(const std::vector<Screen*>& selected);
    void drawModeStack(const std::vector<Screen*>& selected);
    void drawEntryParams(const std::vector<Screen*>& selected, int index, const ModeEntry& entry);
};