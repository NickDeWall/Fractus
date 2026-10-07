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

        if (pointInScreen(x, y, screen)) {
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

    for (Screen* screen : getSelectedScreens()) {
        const SDL_FPoint size = clampSize(screen->getTargetWidth() * scaleFactor,
            screen->getTargetHeight() * scaleFactor);
        screen->startScale(size.x, size.y);
    }
}

SDL_FPoint ScreenManager::clampSizeFree(float screenWidth, float screenHeight) const {
    const float minSize = 10.0f;
    return {
        std::clamp(screenWidth, minSize, width * Config::MAX_SCREEN_RATIO),
        std::clamp(screenHeight, minSize, height * Config::MAX_SCREEN_RATIO)
    };
}

void ScreenManager::beginStretch(SDL_FPoint mousePos) {
    stretchStart = mousePos;
    stretchBaseline.clear();
    for (Screen* screen : getSelectedScreens()) {
        stretchBaseline[screen->getId()] = { screen->getTargetWidth(), screen->getTargetHeight() };
    }
}

void ScreenManager::updateStretch(SDL_FPoint mousePos) {
    const std::vector<Screen*> selected = getSelectedScreens();
    if (selected.empty()) return;

    const auto primary = stretchBaseline.find(selected.front()->getId());
    if (primary == stretchBaseline.end()) return;

    const float minSize = 10.0f;
    const float widthRatio = std::max(minSize, primary->second.x + mousePos.x - stretchStart.x) / primary->second.x;
    const float heightRatio = std::max(minSize, primary->second.y + stretchStart.y - mousePos.y) / primary->second.y;

    for (Screen* screen : selected) {
        const auto base = stretchBaseline.find(screen->getId());
        if (base == stretchBaseline.end()) continue;

        const SDL_FPoint size = clampSizeFree(base->second.x * widthRatio, base->second.y * heightRatio);
        screen->setWidth(size.x);
        screen->setHeight(size.y);
    }
}

void ScreenManager::endStretch() {
    stretchBaseline.clear();
}

SDL_FPoint ScreenManager::clampSize(float screenWidth, float screenHeight) const {
    const float minSize = 10.0f;
    const float maxWidth = width * Config::MAX_SCREEN_RATIO;
    const float maxHeight = height * Config::MAX_SCREEN_RATIO;

    float scale = std::min({ 1.0f, maxWidth / screenWidth, maxHeight / screenHeight });
    scale = std::max({ scale, minSize / screenWidth, minSize / screenHeight });

    return { screenWidth * scale, screenHeight * scale };
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
        screen.setWidth(std::max(1.0f, screen.getTargetWidth() * scaleX));
        screen.setHeight(std::max(1.0f, screen.getTargetHeight() * scaleY));
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

bool ScreenManager::pointInScreen(float px, float py, const Screen& screen) {
    const float angleRad = -screen.getRotation() * Config::PI / 180.0f;
    const float dx = px - screen.getX();
    const float dy = py - screen.getY();
    const float cosTheta = std::cos(angleRad);
    const float sinTheta = std::sin(angleRad);

    const float localX = dx * cosTheta - dy * sinTheta;
    const float localY = dx * sinTheta + dy * cosTheta;
    const float halfWidth = std::max(screen.getWidth() * 0.5f, 0.5f);
    const float halfHeight = std::max(screen.getHeight() * 0.5f, 0.5f);

    return DisplayModes::contains(screen.getStack(), localX / halfWidth, localY / halfHeight);
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