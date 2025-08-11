#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include "screen.h"
#include "config.h"

class ScreenManager {
public:
    ScreenManager(int width, int height);

    Screen* createScreen(SDL_FPoint pos);
    Screen* handleSelection(SDL_FPoint mousePos);
    void handleDragging(SDL_FPoint mousePos);
    void handleScaling(int scrollY);
    void handleRotation(float direction);

    Screen* getSelectedScreen() const { return selectedScreen; }
    const std::vector<Screen>& getScreens() const { return screens; }

private:
    std::vector<Screen> screens;
    Screen* selectedScreen;
    SDL_FPoint dragOffset;
    int width;
    int height;

    Screen* findScreenAtPosition(float x, float y) const;
    Screen* selectSmallestFromCandidates(const std::vector<Screen*>& candidates) const;

    static SDL_FPoint rotatePoint(float cx, float cy, float x, float y, float angle);
    static bool pointInRotatedRect(float px, float py, SDL_FPoint rectCenter,
        float width, float height, float angle);
    static float isLeft(SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint point);
};
