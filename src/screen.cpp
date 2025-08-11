#include "screen.h"
#include <cmath>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>

Screen::Screen(float x, float y, int width, int height, float rotation, SDL_Color color)
    : xCoord(x), yCoord(y), origWidth(width), origHeight(height), rotation(rotation), color(color) {
}

void Screen::setX(float x) {
    xCoord = x;
}

void Screen::setY(float y) {
    yCoord = y;
}

void Screen::setWidth(int width) {
    origWidth = width;
}

void Screen::setHeight(int height) {
    origHeight = height;
}

void Screen::setRotation(float rot) {
    rotation = rot;
}

void Screen::setColor(SDL_Color col) {
    color = col;
}

float Screen::getX() const {
    return xCoord;
}

float Screen::getY() const {
    return yCoord;
}

int Screen::getWidth() const {
    return origWidth;
}

int Screen::getHeight() const {
    return origHeight;
}

float Screen::getRotation() const {
    return rotation;
}

SDL_Color Screen::getColor() const {
    return color;
}

SDL_Color Screen::getOutlineColor() const {
    return { color.r, color.g, color.b, Config::OUTLINE_ALPHA };
}

SDL_Color Screen::getScaleOutlineColor() const {
    return { color.r, color.g, color.b,
             static_cast<Uint8>(std::min(255, static_cast<int>(Config::OUTLINE_ALPHA + Config::OUTLINE_SCALE_INCREASE))) };
}

void Screen::rotate(float degrees) {
    rotation = fmod(rotation + degrees, 360.0f);
}

SDL_FPoint Screen::getRotatedSize() const {
    float w = static_cast<float>(origWidth);
    float h = static_cast<float>(origHeight);
    float angleRad = rotation * Config::PI / 180.0f;

    float cos_a = std::abs(std::cos(angleRad));
    float sin_a = std::abs(std::sin(angleRad));
    
    float newW = w * cos_a + h * sin_a;
    float newH = w * sin_a + h * cos_a;
    
    return { newW, newH };
}
