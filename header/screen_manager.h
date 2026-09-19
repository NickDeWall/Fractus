#pragma once
#include <vector>
#include <unordered_set>
#include <SDL2/SDL.h>
#include "screen.h"
#include "config.h"

class ScreenManager {
public:
    ScreenManager(int width, int height);

    void createScreen(SDL_FPoint pos);
    void handleSelection(SDL_FPoint mousePos);
    void clearSelection();
    void deleteSelected();
    void handleDragging(SDL_FPoint mousePos);
    void handleScaling(int scrollY);
    void update(float dt, int rotateInput);
    void resize(int newWidth, int newHeight);

    bool isSelected(const Screen& screen) const;
    std::vector<Screen*> getSelectedScreens();
    const std::vector<Screen>& getScreens() const { return screens; }

private:
    std::vector<Screen> screens;
    std::unordered_set<int> selectedIds;
    int nextId;
    SDL_FPoint lastMousePos;
    int width;
    int height;

    Screen* findScreenAtPosition(float x, float y) const;
    Screen* selectSmallestFromCandidates(const std::vector<Screen*>& candidates) const;

    static SDL_FPoint rotatePoint(float cx, float cy, float x, float y, float angle);
    static bool pointInRotatedRect(float px, float py, SDL_FPoint rectCenter,
        float width, float height, float angle);
    static float isLeft(SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint point);
};