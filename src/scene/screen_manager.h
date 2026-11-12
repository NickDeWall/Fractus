#pragma once
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <SDL2/SDL.h>
#include "scene/screen.h"
#include "core/config.h"

class ScreenManager {
public:
    ScreenManager(int width, int height);

    void createScreen(SDL_FPoint pos);
    void handleSelection(SDL_FPoint mousePos, bool additive);
    void handleDeleteClick(SDL_FPoint mousePos);
    void updatePointer(SDL_FPoint mousePos, bool enabled);
    void endSelection(SDL_FPoint mousePos, bool additive);
    void clearSelection();
    void deleteSelected();
    void handleScaling(int scrollY);

    bool isBoxSelecting() const { return boxActive; }
    SDL_FPoint getBoxStart() const { return boxStart; }
    SDL_FPoint getBoxEnd() const { return boxEnd; }
    void setGroupPivot(bool enabled) { groupPivot = enabled; }
    bool getGroupPivot() const { return groupPivot; }
    bool usesGroupPivot() const { return groupPivot && selectedIds.size() > 1; }
    SDL_FPoint selectionCenter() const;
    void update(float dt, int rotateInput);
    void resize(int newWidth, int newHeight);

    SDL_FPoint clampSize(float screenWidth, float screenHeight) const;
    SDL_FPoint clampSizeFree(float screenWidth, float screenHeight) const;
    void beginStretch(SDL_FPoint mousePos);
    void updateStretch(SDL_FPoint mousePos);
    void endStretch();
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool isSelected(const Screen& screen) const;
    std::vector<Screen*> getSelectedScreens();
    const std::vector<Screen>& getScreens() const { return screens; }
    float surfaceSpeed(float x, float y, float normalX, float normalY) const;

private:
    std::vector<Screen> screens;
    std::unordered_set<int> selectedIds;
    int primaryId;
    int nextId;
    bool boxActive;
    bool draggingScreens;
    bool dragMoved;
    int pressedId;
    std::unordered_set<int> boxBase;
    SDL_FPoint pressPos;
    bool groupPivot;
    SDL_FPoint boxStart;
    SDL_FPoint boxEnd;
    SDL_FPoint lastMousePos;
    SDL_FPoint stretchStart;
    std::unordered_map<int, SDL_FPoint> stretchBaseline;
    int width;
    int height;

    Screen* findScreenAtPosition(float x, float y) const;
    void selectOnly(int id);
    void applyBox();
    static bool screenOverlapsBox(const Screen& screen, SDL_FPoint corner, SDL_FPoint opposite);
    void refreshPrimary();
    Screen* selectSmallestFromCandidates(const std::vector<Screen*>& candidates) const;

    static bool pointInScreen(float px, float py, const Screen& screen);
    static SDL_FPoint rotatePoint(float cx, float cy, float x, float y, float angle);
    static bool pointInRotatedRect(float px, float py, SDL_FPoint rectCenter,
        float width, float height, float angle);
    static float isLeft(SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint point);
};