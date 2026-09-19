#pragma once
#include <SDL2/SDL.h>
#include <vector>

class Screen;

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

    void render(const std::vector<Screen*>& selected);

private:
    bool screenMenuOpen = false;

    void drawScreenMenu(const std::vector<Screen*>& selected);
};