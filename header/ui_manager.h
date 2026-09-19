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
    std::unordered_map<int, SDL_Point> sizeBaseline;

    void drawScreenMenu(ScreenManager& screenManager, const std::vector<Screen*>& selected);
    void drawSizeSlider(ScreenManager& screenManager, const std::vector<Screen*>& selected);
};