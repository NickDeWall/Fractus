#include "screen.h"
#include <cmath>
#include <algorithm>
#include <math_utils.h>

#define _USE_MATH_DEFINES
#include <math.h>

Screen::Screen(float x, float y, int width, int height, float rotation, SDL_Color color)
    : xCoord(x), yCoord(y), origWidth(width), origHeight(height), rotation(rotation), color(color) {
    trueR = static_cast<float>(color.r);
    trueG = static_cast<float>(color.g);
    trueB = static_cast<float>(color.b);
    trueA = static_cast<float>(color.a);
    MathUtils::rgbToHsv(trueR, trueG, trueB, trueH, trueS, trueV);
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

void Screen::setTrueR(float r) {
    trueR = r;
}

void Screen::setTrueG(float g) {
    trueG = g;
}

void Screen::setTrueB(float b) {
    trueB = b;
}

void Screen::setTrueA(float a) {
    trueA = a;
}

void Screen::setTrueH(float h) {
    trueH = h;
}

void Screen::setTrueS(float s) {
    trueS = s;
}

void Screen::setTrueV(float v) {
    trueV = v;
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

float Screen::getTrueR() const {
    return trueR;
}

float Screen::getTrueG() const {
    return trueG;
}

float Screen::getTrueB() const {
    return trueB;
}

float Screen::getTrueA() const {
    return trueA;
}

float Screen::getTrueH() const {
    return trueH; 
}

float Screen::getTrueS() const {
    return trueS; 
}

float Screen::getTrueV() const {
    return trueV; 
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
