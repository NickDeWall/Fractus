#pragma once
#include <SDL2/SDL.h>
#include "config.h"

class Screen {
public:
    Screen(float x, float y, int width, int height, float rotation, SDL_Color color);

    // Setters
    void setX(float x);
    void setY(float y);
    void setWidth(int width);
    void setHeight(int height);
    void setRotation(float rotation);
    void setColor(SDL_Color color);
    void setTrueR(float r);
    void setTrueG(float g);
    void setTrueB(float b);
    void setTrueA(float a);
    void setTrueH(float h);
    void setTrueS(float s);
    void setTrueV(float v);

    // Getters
    float getX() const;
    float getY() const;
    int getWidth() const;
    int getHeight() const;
    float getRotation() const;
    SDL_Color getColor() const;
    float getTrueR() const;
    float getTrueG() const;
    float getTrueB() const;
    float getTrueA() const;
    float getTrueH() const;
    float getTrueS() const;
    float getTrueV() const;
    SDL_Color getOutlineColor() const;
    SDL_Color getScaleOutlineColor() const;

    // Actions
    void rotate(float degrees);
    SDL_FPoint getRotatedSize() const;

private:
    float xCoord;
    float yCoord;
    int origWidth;
    int origHeight;
    float rotation;

    float trueR;
    float trueG;
    float trueB;
    float trueA;

    float trueH;
    float trueS;
    float trueV;
    
    SDL_Color color;
};
