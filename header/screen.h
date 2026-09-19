#pragma once
#include <SDL2/SDL.h>
#include "config.h"

class Screen {
public:
    Screen(int id, float x, float y, int width, int height, float rotation, SDL_Color color);

    // Setters
    void setX(float x);
    void setY(float y);
    void setWidth(int width);
    void setHeight(int height);
    void setRotation(float rotation);
    void setHue(float h);
    void setSaturation(float s);
    void setValue(float v);
    void setAlpha(float a);

    // Getters
    int getId() const;
    float getX() const;
    float getY() const;
    int getWidth() const;
    int getHeight() const;
    float getRotation() const;
    SDL_Color getColor() const;
    float getHue() const;
    float getSaturation() const;
    float getValue() const;
    float getAlpha() const;
    SDL_Color getOutlineColor() const;
    SDL_Color getScaleOutlineColor() const;
    int getTargetWidth() const;
    float getTargetX() const;
    float getTargetY() const;
    int getTargetHeight() const;

    // Actions
    void rotate(float degrees);
    SDL_FPoint getRotatedSize() const;
    void startScale(int width, int height);
    void moveTo(float x, float y);
    void update(float dt, float targetVelocity);

private:
    int id;
    float xCoord;
    float yCoord;
    int origWidth;
    int origHeight;
    float rotation;

    float hue;
    float saturation;
    float value;
    float alpha;

    int fromWidth;
    int fromHeight;
    int targetWidth;
    int targetHeight;
    float scaleProgress;
    float angularVelocity;
    float targetX;
    float targetY;
};