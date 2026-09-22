#pragma once
#include <SDL2/SDL.h>
#include "config.h"

enum class DisplayMode {
    Normal = 0,
    Prop = 1,
    LogPolar = 2,
    Julia = 3,
    Droste = 4,
    Power = 5,
    Kaleidoscope = 6,
    Inversion = 7,
    Swirl = 8,
    Count
};

class Screen {
public:
    Screen(int id, float x, float y, int width, int height, float rotation, SDL_Color color);

    // Setters
    void setX(float x);
    void setY(float y);
    void setWidth(float width);
    void setHeight(float height);
    void setRotation(float rotation);
    void setHue(float h);
    void setSaturation(float s);
    void setValue(float v);
    void setAlpha(float a);
    void setUpdateRate(float rate);
    void setDelay(float seconds);
    void setDisplayMode(DisplayMode mode);
    void setLogPolarMinRadius(float radius);
    void setJuliaC(float real, float imag);
    void setDrosteZoom(float zoom);
    void setDrosteArms(int arms);
    void setPower(float power);
    void setKaleidoscopeSegments(int segments);
    void setKaleidoscopeAngle(float degrees);
    void setInversionRadius(float radius);
    void setSwirlStrength(float turns);
    void setSwirlRadius(float radius);

    // Getters
    int getId() const;
    float getX() const;
    float getY() const;
    int getWidth() const;
    int getHeight() const;
    float getRotation() const;
    void getColorF(float& r, float& g, float& b, float& a) const;
    float getHue() const;
    float getSaturation() const;
    float getValue() const;
    float getAlpha() const;
    float getUpdateRate() const;
    float getDelay() const;
    DisplayMode getDisplayMode() const;
    float getLogPolarMinRadius() const;
    float getJuliaReal() const;
    float getJuliaImag() const;
    float getDrosteZoom() const;
    int getDrosteArms() const;
    float getPower() const;
    int getKaleidoscopeSegments() const;
    float getKaleidoscopeAngle() const;
    float getInversionRadius() const;
    float getSwirlStrength() const;
    float getSwirlRadius() const;
    SDL_Color getOutlineColor() const;
    SDL_Color getScaleOutlineColor() const;
    float getTargetWidth() const;
    float getTargetX() const;
    float getTargetY() const;
    float getTargetHeight() const;

    // Actions
    void rotate(float degrees);
    SDL_FPoint getRotatedSize() const;
    void startScale(float width, float height);
    void moveTo(float x, float y);
    void update(float dt, float targetVelocity);

private:
    int id;
    float xCoord;
    float yCoord;
    float origWidth;
    float origHeight;
    float rotation;

    float hue;
    float saturation;
    float value;
    float alpha;

    float fromWidth;
    float fromHeight;
    float targetWidth;
    float targetHeight;
    float scaleProgress;
    float angularVelocity;
    float targetX;
    float targetY;
    float updateRate;
    float delay;
    DisplayMode displayMode;
    float logPolarMinRadius;
    float juliaReal;
    float juliaImag;
    float drosteZoom;
    int drosteArms;
    float power;
    int kaleidoscopeSegments;
    float kaleidoscopeAngle;
    float inversionRadius;
    float swirlStrength;
    float swirlRadius;
};