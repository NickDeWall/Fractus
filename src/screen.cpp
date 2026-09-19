#include "screen.h"
#include <cmath>
#include <algorithm>
#include <math_utils.h>

#define _USE_MATH_DEFINES
#include <math.h>

Screen::Screen(int id, float x, float y, int width, int height, float rotation, SDL_Color color)
    : id(id), xCoord(x), yCoord(y), origWidth(width), origHeight(height), rotation(rotation),
      fromWidth(width), fromHeight(height), targetWidth(width), targetHeight(height), scaleProgress(1.0f), angularVelocity(0.0f), targetX(x), targetY(y) {
    MathUtils::rgbToHsv(color.r, color.g, color.b, hue, saturation, value);
    alpha = color.a / 255.0f;
}

void Screen::setX(float x) {
    xCoord = targetX = x;
}

void Screen::setY(float y) {
    yCoord = targetY = y;
}

void Screen::setWidth(float width) {
    origWidth = fromWidth = targetWidth = width;
}

void Screen::setHeight(float height) {
    origHeight = fromHeight = targetHeight = height;
}

void Screen::setRotation(float rot) {
    rotation = rot;
    angularVelocity = 0.0f;
}

void Screen::setHue(float h) {
    hue = h - std::floor(h);
}

void Screen::setSaturation(float s) {
    saturation = std::clamp(s, 0.0f, 1.0f);
}

void Screen::setValue(float v) {
    value = std::clamp(v, 0.0f, 1.0f);
}

void Screen::setAlpha(float a) {
    alpha = std::clamp(a, 0.0f, 1.0f);
}

int Screen::getId() const {
    return id;
}

float Screen::getX() const {
    return xCoord;
}

float Screen::getY() const {
    return yCoord;
}

int Screen::getWidth() const {
    return static_cast<int>(std::lround(origWidth));
}

int Screen::getHeight() const {
    return static_cast<int>(std::lround(origHeight));
}

float Screen::getRotation() const {
    return rotation;
}

void Screen::getColorF(float& r, float& g, float& b, float& a) const {
    MathUtils::hsvToRgb(hue, saturation, value, r, g, b);
    r /= 255.0f;
    g /= 255.0f;
    b /= 255.0f;
    a = alpha;
}

float Screen::getHue() const {
    return hue;
}

float Screen::getSaturation() const {
    return saturation;
}

float Screen::getValue() const {
    return value;
}

float Screen::getAlpha() const {
    return alpha;
}

SDL_Color Screen::getOutlineColor() const {
    float r, g, b;
    MathUtils::hsvToRgb(hue, saturation, std::max(value, Config::OUTLINE_MIN_VALUE), r, g, b);
    return {
        static_cast<Uint8>(std::lround(r)),
        static_cast<Uint8>(std::lround(g)),
        static_cast<Uint8>(std::lround(b)),
        Config::OUTLINE_ALPHA
    };
}


SDL_Color Screen::getScaleOutlineColor() const {
    const SDL_Color color = getOutlineColor();
    return { color.r, color.g, color.b,
             static_cast<Uint8>(std::min(255, static_cast<int>(Config::OUTLINE_ALPHA + Config::OUTLINE_SCALE_INCREASE))) };
}

float Screen::getTargetWidth() const {
    return targetWidth;
}

float Screen::getTargetHeight() const {
    return targetHeight;
}

float Screen::getTargetX() const {
    return targetX;
}

float Screen::getTargetY() const {
    return targetY;
}

void Screen::rotate(float degrees) {
    rotation = fmod(rotation + degrees, 360.0f);
}

SDL_FPoint Screen::getRotatedSize() const {
    float w = origWidth;
    float h = origHeight;
    float angleRad = rotation * Config::PI / 180.0f;

    float cos_a = std::abs(std::cos(angleRad));
    float sin_a = std::abs(std::sin(angleRad));
    
    float newW = w * cos_a + h * sin_a;
    float newH = w * sin_a + h * cos_a;
    
    return { newW, newH };
}

void Screen::startScale(float width, float height) {
    fromWidth = origWidth;
    fromHeight = origHeight;
    targetWidth = width;
    targetHeight = height;
    scaleProgress = 0.0f;
}

void Screen::moveTo(float x, float y) {
    targetX = x;
    targetY = y;
}

void Screen::update(float dt, float targetVelocity) {
    const bool braking = targetVelocity == 0.0f || targetVelocity * angularVelocity < 0.0f;
    const float step = Config::ROTATION_SPEED / (braking ? Config::ROTATION_DECEL_TIME : Config::ROTATION_RAMP_TIME) * dt;
    angularVelocity += std::clamp(targetVelocity - angularVelocity, -step, step);
    if (angularVelocity != 0.0f) rotate(angularVelocity * dt);

    const double follow = 1.0 - std::exp(-dt / Config::MOVE_SMOOTH_TIME);
    xCoord = static_cast<float>(MathUtils::linearInterpolate(xCoord, targetX, follow));
    yCoord = static_cast<float>(MathUtils::linearInterpolate(yCoord, targetY, follow));

    if (scaleProgress >= 1.0f) return;
    scaleProgress = std::min(1.0f, scaleProgress + dt / Config::SCALE_DURATION);
    origWidth = static_cast<float>(MathUtils::easeOutPowerInterpolate(fromWidth, targetWidth, scaleProgress, Config::SCALE_EASE_STRENGTH));
    origHeight = static_cast<float>(MathUtils::easeOutPowerInterpolate(fromHeight, targetHeight, scaleProgress, Config::SCALE_EASE_STRENGTH));
}