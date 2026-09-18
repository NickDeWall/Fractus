#include "screen_manager.h"
#include <cmath>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include "config.h"

ScreenManager::ScreenManager(int width, int height)
    : selectedScreen(nullptr), width(width), height(height) {
    dragOffset = { 0, 0 };
}

Screen* ScreenManager::createScreen(SDL_FPoint pos) {
    int initialWidth = static_cast<int>(width * Config::INITIAL_SCREEN_SIZE_RATIO);
    int initialHeight = static_cast<int>(height * Config::INITIAL_SCREEN_SIZE_RATIO);

    screens.emplace_back(pos.x, pos.y, initialWidth, initialHeight, 0,
        Config::DEFAULT_SCREEN_COLOR);
    return &screens.back();
}

Screen* ScreenManager::handleSelection(SDL_FPoint mousePos) {
    selectedScreen = findScreenAtPosition(mousePos.x, mousePos.y);
    if (selectedScreen) {
        dragOffset = {
            mousePos.x - selectedScreen->getX(),
            mousePos.y - selectedScreen->getY()
        };
    }
    return selectedScreen;
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

void ScreenManager::handleDragging(SDL_FPoint mousePos) {
    if (selectedScreen && (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK)) {
        float newX = mousePos.x - dragOffset.x;
        float newY = mousePos.y - dragOffset.y;

        selectedScreen->setX(newX);
        selectedScreen->setY(newY);
    }
}

void ScreenManager::handleScaling(int scrollY) {
    if (selectedScreen && scrollY) {
        float scaleFactor = (scrollY > 0) ? Config::SCALE_FACTOR_UP : Config::SCALE_FACTOR_DOWN;

        int newWidth = static_cast<int>(selectedScreen->getTargetWidth() * scaleFactor);
        int newHeight = static_cast<int>(selectedScreen->getTargetHeight() * scaleFactor);

        newWidth = std::max(10, std::min(newWidth, static_cast<int>(width * Config::MAX_SCREEN_RATIO)));
        newHeight = std::max(10, std::min(newHeight, static_cast<int>(height * Config::MAX_SCREEN_RATIO)));

        selectedScreen->startScale(newWidth, newHeight);
    }
}

void ScreenManager::update(float dt, int rotateInput) {
    for (auto& screen : screens) {
        const float target = (&screen == selectedScreen) ? rotateInput * Config::ROTATION_SPEED : 0.0f;
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
        screen.setX(screen.getX() * scaleX);
        screen.setY(screen.getY() * scaleY);
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