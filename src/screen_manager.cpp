#include "screen_manager.h"
#include <cmath>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include "config.h"

ScreenManager::ScreenManager(int width, int height)
    : nextId(0), width(width), height(height) {
    lastMousePos = { 0, 0 };
}

void ScreenManager::createScreen(SDL_FPoint pos) {
    int initialWidth = static_cast<int>(width * Config::INITIAL_SCREEN_SIZE_RATIO);
    int initialHeight = static_cast<int>(height * Config::INITIAL_SCREEN_SIZE_RATIO);

    screens.emplace_back(nextId++, pos.x, pos.y, initialWidth, initialHeight, 0,
        Config::DEFAULT_SCREEN_COLOR);
}

void ScreenManager::handleSelection(SDL_FPoint mousePos) {
    selectedIds.clear();
    lastMousePos = mousePos;
    if (Screen* hit = findScreenAtPosition(mousePos.x, mousePos.y)) {
        selectedIds.insert(hit->getId());
    }
}

void ScreenManager::clearSelection() {
    selectedIds.clear();
}

void ScreenManager::deleteSelected() {
    screens.erase(std::remove_if(screens.begin(), screens.end(),
        [this](const Screen& s) { return isSelected(s); }), screens.end());
    selectedIds.clear();
}

bool ScreenManager::isSelected(const Screen& screen) const {
    return selectedIds.count(screen.getId()) > 0;
}

std::vector<Screen*> ScreenManager::getSelectedScreens() {
    std::vector<Screen*> selected;
    for (auto& screen : screens) {
        if (isSelected(screen)) {
            selected.push_back(&screen);
        }
    }
    return selected;
}

Screen* ScreenManager::findScreenAtPosition(float x, float y) const {
    std::vector<Screen*> candidates;

    for (auto it = screens.rbegin(); it != screens.rend(); ++it) {
        const Screen& screen = *it;

        if (pointInRotatedRect(x, y,
            { screen.getX(), screen.getY() },
            screen.getWidth(), screen.getHeight(),
            screen.getRotation())) {
            candidates.push_back(const_cast<Screen*>(&screen));
        }
    }

    return selectSmallestFromCandidates(candidates);
}

Screen* ScreenManager::selectSmallestFromCandidates(const std::vector<Screen*>& candidates) const {
    if (candidates.empty()) {
        return nullptr;
    }

    return *std::min_element(candidates.begin(), candidates.end(),
        [this](Screen* a, Screen* b) {
            int areaA = a->getWidth() * a->getHeight();
            int areaB = b->getWidth() * b->getHeight();

            if (areaA != areaB) {
                return areaA < areaB;
            }

            auto distA = std::distance(screens.begin(),
                std::find_if(screens.begin(), screens.end(),
                    [a](const Screen& s) { return &s == a; }));
            auto distB = std::distance(screens.begin(),
                std::find_if(screens.begin(), screens.end(),
                    [b](const Screen& s) { return &s == b; }));
            return distB < distA;
        });
}

void ScreenManager::handleDragging(SDL_FPoint mousePos, bool enabled) {
    const float dx = mousePos.x - lastMousePos.x;
    const float dy = mousePos.y - lastMousePos.y;
    lastMousePos = mousePos;

    if (!enabled || !(SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK)) return;

    for (Screen* screen : getSelectedScreens()) {
        screen->moveTo(screen->getTargetX() + dx, screen->getTargetY() + dy);
    }
}

void ScreenManager::handleScaling(int scrollY) {
    if (!scrollY) return;

    const float scaleFactor = (scrollY > 0) ? Config::SCALE_FACTOR_UP : Config::SCALE_FACTOR_DOWN;
    const int maxWidth = static_cast<int>(width * Config::MAX_SCREEN_RATIO);
    const int maxHeight = static_cast<int>(height * Config::MAX_SCREEN_RATIO);

    for (Screen* screen : getSelectedScreens()) {
        int newWidth = static_cast<int>(screen->getTargetWidth() * scaleFactor);
        int newHeight = static_cast<int>(screen->getTargetHeight() * scaleFactor);

        newWidth = std::max(10, std::min(newWidth, maxWidth));
        newHeight = std::max(10, std::min(newHeight, maxHeight));

        screen->startScale(newWidth, newHeight);
    }
}

void ScreenManager::update(float dt, int rotateInput) {
    for (auto& screen : screens) {
        const float target = isSelected(screen) ? rotateInput * Config::ROTATION_SPEED : 0.0f;
        screen.update(dt, target);
    }
}

void ScreenManager::resize(int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return;
    if (width <= 0 || height <= 0) {
        width = newWidth;
        height = newHeight;
        return;
    }
    if (newWidth == width && newHeight == height) return;

    const float scaleX = static_cast<float>(newWidth) / static_cast<float>(width);
    const float scaleY = static_cast<float>(newHeight) / static_cast<float>(height);

    for (auto& screen : screens) {
        screen.setX(screen.getTargetX() * scaleX);
        screen.setY(screen.getTargetY() * scaleY);
        screen.setWidth(std::max(1, static_cast<int>(std::lround(screen.getTargetWidth() * scaleX))));
        screen.setHeight(std::max(1, static_cast<int>(std::lround(screen.getTargetHeight() * scaleY))));
    }

    width = newWidth;
    height = newHeight;
}

SDL_FPoint ScreenManager::rotatePoint(float cx, float cy, float x, float y, float angle) {
    float angleRad = angle * Config::PI / 180.0f;
    float dx = x - cx;
    float dy = y - cy;
    float cosTheta = std::cos(angleRad);
    float sinTheta = std::sin(angleRad);

    float xNew = dx * cosTheta - dy * sinTheta;
    float yNew = dx * sinTheta + dy * cosTheta;

    return { cx + xNew, cy + yNew };
}

bool ScreenManager::pointInRotatedRect(float px, float py, SDL_FPoint rectCenter, float width, float height, float angle) {
    float angleRad = -angle * Config::PI / 180.0f;
    float dx = px - rectCenter.x;
    float dy = py - rectCenter.y;
    
    float cosTheta = std::cos(angleRad);
    float sinTheta = std::sin(angleRad);
    
    float localX = dx * cosTheta - dy * sinTheta;
    float localY = dx * sinTheta + dy * cosTheta;
    
    return (localX >= -width/2 && localX <= width/2 && 
            localY >= -height/2 && localY <= height/2);
}

float ScreenManager::isLeft(SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint point) {
    return (p1.x - p0.x) * (point.y - p0.y) - (point.x - p0.x) * (p1.y - p0.y);
}