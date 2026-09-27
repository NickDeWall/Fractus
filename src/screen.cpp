#include "screen.h"
#include <cmath>
#include <algorithm>
#include <math_utils.h>

#define _USE_MATH_DEFINES
#include <math.h>

Screen::Screen(int id, float x, float y, int width, int height, float rotation, SDL_Color color)
    : id(id), xCoord(x), yCoord(y), origWidth(width), origHeight(height), rotation(rotation),
      fromWidth(width), fromHeight(height), targetWidth(width), targetHeight(height), scaleProgress(1.0f), angularVelocity(0.0f), targetX(x), targetY(y), updateRate(Config::DEFAULT_UPDATE_RATE), delay(0.0f), displayMode(DisplayMode::Normal),
      logPolarMinRadius(Config::LOG_POLAR_MIN_RADIUS),
      juliaReal(Config::JULIA_DEFAULT_REAL), juliaImag(Config::JULIA_DEFAULT_IMAG),
      drosteZoom(Config::DROSTE_DEFAULT_ZOOM), drosteArms(Config::DROSTE_DEFAULT_ARMS),
      power(Config::POWER_DEFAULT),
      kaleidoscopeSegments(Config::KALEIDOSCOPE_DEFAULT_SEGMENTS), kaleidoscopeAngle(0.0f),
      inversionRadius(Config::INVERSION_DEFAULT_RADIUS),
      swirlStrength(Config::SWIRL_DEFAULT_STRENGTH), swirlRadius(Config::SWIRL_DEFAULT_RADIUS),
      tileCount(Config::TILE_DEFAULT_COUNT), tileMirror(true),
      sharpenStrength(Config::SHARPEN_DEFAULT_STRENGTH), hueShift(Config::HUE_SHIFT_DEFAULT),
      mobiusBReal(Config::MOBIUS_DEFAULT_B_REAL), mobiusBImag(Config::MOBIUS_DEFAULT_B_IMAG),
      mobiusCReal(Config::MOBIUS_DEFAULT_C_REAL), mobiusCImag(Config::MOBIUS_DEFAULT_C_IMAG),
      invertAmount(Config::INVERT_DEFAULT_AMOUNT), invertContrast(Config::INVERT_DEFAULT_CONTRAST), invertHue(true),
      chromaticSplit(Config::CHROMATIC_DEFAULT_SPLIT),
      newtonOrder(Config::NEWTON_DEFAULT_ORDER), newtonStep(Config::NEWTON_DEFAULT_STEP),
      shearX(0.2f), shearY(0.0f) {
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

void Screen::setUpdateRate(float rate) {
    updateRate = std::clamp(rate, Config::MIN_UPDATE_RATE, Config::MAX_UPDATE_RATE);
}

void Screen::setDelay(float seconds) {
    delay = std::clamp(seconds, 0.0f, Config::MAX_DELAY);
}

void Screen::setDisplayMode(DisplayMode mode) {
    displayMode = mode;
}

void Screen::setLogPolarMinRadius(float radius) {
    logPolarMinRadius = std::clamp(radius, Config::LOG_POLAR_MIN_RADIUS_LOW, Config::LOG_POLAR_MIN_RADIUS_HIGH);
}

void Screen::setJuliaC(float real, float imag) {
    juliaReal = std::clamp(real, -Config::JULIA_C_LIMIT, Config::JULIA_C_LIMIT);
    juliaImag = std::clamp(imag, -Config::JULIA_C_LIMIT, Config::JULIA_C_LIMIT);
}

void Screen::setDrosteZoom(float zoom) {
    drosteZoom = std::clamp(zoom, Config::DROSTE_MIN_ZOOM, Config::DROSTE_MAX_ZOOM);
}

void Screen::setDrosteArms(int arms) {
    drosteArms = std::clamp(arms, -Config::DROSTE_ARM_LIMIT, Config::DROSTE_ARM_LIMIT);
}

void Screen::setPower(float value) {
    power = std::clamp(value, -Config::POWER_LIMIT, Config::POWER_LIMIT);
}

void Screen::setKaleidoscopeSegments(int segments) {
    kaleidoscopeSegments = std::clamp(segments, Config::KALEIDOSCOPE_MIN_SEGMENTS, Config::KALEIDOSCOPE_MAX_SEGMENTS);
}

void Screen::setKaleidoscopeAngle(float degrees) {
    kaleidoscopeAngle = degrees - 360.0f * std::floor(degrees / 360.0f);
}

void Screen::setInversionRadius(float radius) {
    inversionRadius = std::clamp(radius, Config::INVERSION_MIN_RADIUS, Config::INVERSION_MAX_RADIUS);
}

void Screen::setSwirlStrength(float turns) {
    swirlStrength = std::clamp(turns, -Config::SWIRL_STRENGTH_LIMIT, Config::SWIRL_STRENGTH_LIMIT);
}

void Screen::setSwirlRadius(float radius) {
    swirlRadius = std::clamp(radius, Config::SWIRL_MIN_RADIUS, Config::SWIRL_MAX_RADIUS);
}

void Screen::setTileCount(int tiles) {
    tileCount = std::clamp(tiles, Config::TILE_MIN_COUNT, Config::TILE_MAX_COUNT);
}

void Screen::setTileMirror(bool mirror) {
    tileMirror = mirror;
}

void Screen::setSharpenStrength(float strength) {
    sharpenStrength = std::clamp(strength, 0.0f, Config::SHARPEN_STRENGTH_LIMIT);
}

void Screen::setHueShift(float turns) {
    hueShift = std::clamp(turns, -Config::HUE_SHIFT_LIMIT, Config::HUE_SHIFT_LIMIT);
}

void Screen::setMobiusB(float real, float imag) {
    mobiusBReal = std::clamp(real, -Config::MOBIUS_B_LIMIT, Config::MOBIUS_B_LIMIT);
    mobiusBImag = std::clamp(imag, -Config::MOBIUS_B_LIMIT, Config::MOBIUS_B_LIMIT);
}

void Screen::setInvertAmount(float amount) {
    invertAmount = std::clamp(amount, 0.0f, 1.0f);
}

void Screen::setInvertContrast(float contrast) {
    invertContrast = std::clamp(contrast, Config::INVERT_MIN_CONTRAST, Config::INVERT_MAX_CONTRAST);
}

void Screen::setInvertHue(bool hueFlip) {
    invertHue = hueFlip;
}

void Screen::setChromaticSplit(float split) {
    chromaticSplit = std::clamp(split, -Config::CHROMATIC_SPLIT_LIMIT, Config::CHROMATIC_SPLIT_LIMIT);
}

void Screen::setNewtonOrder(int order) {
    newtonOrder = std::clamp(order, Config::NEWTON_MIN_ORDER, Config::NEWTON_MAX_ORDER);
}

void Screen::setNewtonStep(float step) {
    newtonStep = std::clamp(step, Config::NEWTON_MIN_STEP, Config::NEWTON_MAX_STEP);
}

void Screen::setShear(float x, float y) {
    shearX = std::clamp(x, -Config::SHEAR_LIMIT, Config::SHEAR_LIMIT);
    shearY = std::clamp(y, -Config::SHEAR_LIMIT, Config::SHEAR_LIMIT);
}

void Screen::setMobiusC(float real, float imag) {
    mobiusCReal = std::clamp(real, -Config::MOBIUS_C_LIMIT, Config::MOBIUS_C_LIMIT);
    mobiusCImag = std::clamp(imag, -Config::MOBIUS_C_LIMIT, Config::MOBIUS_C_LIMIT);
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

float Screen::getUpdateRate() const {
    return updateRate;
}

float Screen::getDelay() const {
    return delay;
}

DisplayMode Screen::getDisplayMode() const {
    return displayMode;
}

float Screen::getLogPolarMinRadius() const {
    return logPolarMinRadius;
}

float Screen::getJuliaReal() const {
    return juliaReal;
}

float Screen::getJuliaImag() const {
    return juliaImag;
}

float Screen::getDrosteZoom() const {
    return drosteZoom;
}

int Screen::getDrosteArms() const {
    return drosteArms;
}

float Screen::getPower() const {
    return power;
}

int Screen::getKaleidoscopeSegments() const {
    return kaleidoscopeSegments;
}

float Screen::getKaleidoscopeAngle() const {
    return kaleidoscopeAngle;
}

float Screen::getInversionRadius() const {
    return inversionRadius;
}

float Screen::getSwirlStrength() const {
    return swirlStrength;
}

float Screen::getSwirlRadius() const {
    return swirlRadius;
}

int Screen::getTileCount() const {
    return tileCount;
}

bool Screen::getTileMirror() const {
    return tileMirror;
}

float Screen::getSharpenStrength() const {
    return sharpenStrength;
}

float Screen::getHueShift() const {
    return hueShift;
}

float Screen::getMobiusBReal() const {
    return mobiusBReal;
}

float Screen::getMobiusBImag() const {
    return mobiusBImag;
}

float Screen::getMobiusCReal() const {
    return mobiusCReal;
}

float Screen::getMobiusCImag() const {
    return mobiusCImag;
}

float Screen::getInvertAmount() const {
    return invertAmount;
}

float Screen::getInvertContrast() const {
    return invertContrast;
}

bool Screen::getInvertHue() const {
    return invertHue;
}

float Screen::getChromaticSplit() const {
    return chromaticSplit;
}

int Screen::getNewtonOrder() const {
    return newtonOrder;
}

float Screen::getNewtonStep() const {
    return newtonStep;
}

float Screen::getShearX() const {
    return shearX;
}

float Screen::getShearY() const {
    return shearY;
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