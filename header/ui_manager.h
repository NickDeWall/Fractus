#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <unordered_map>

class Screen;
class ScreenManager;

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

    void render(ScreenManager& screenManager);

private:
    bool screenMenuOpen = false;

    bool sizeEditing = false;
    float sizeBasePercent = 0.0f;
    std::unordered_map<int, SDL_FPoint> sizeBaseline;

    bool rotationEditing = false;
    float rotationBaseAngle = 0.0f;
    std::unordered_map<int, float> rotationBaseline;

    void drawScreenMenu(ScreenManager& screenManager, const std::vector<Screen*>& selected);
    void drawSizeSlider(ScreenManager& screenManager, const std::vector<Screen*>& selected);
    void drawRotationControl(const std::vector<Screen*>& selected);
    void drawRateSlider(const std::vector<Screen*>& selected);
    void drawDelaySlider(const std::vector<Screen*>& selected);
    void drawDisplayModeCombo(const std::vector<Screen*>& selected);
    void drawLogPolarSlider(const std::vector<Screen*>& selected);
    void drawJuliaSliders(const std::vector<Screen*>& selected);
    void drawDrosteSliders(const std::vector<Screen*>& selected);
    void drawPowerSlider(const std::vector<Screen*>& selected);
    void drawKaleidoscopeSliders(const std::vector<Screen*>& selected);
    void drawInversionSlider(const std::vector<Screen*>& selected);
    void drawSwirlSliders(const std::vector<Screen*>& selected);
};