#pragma once
#include <functional>

class Ball {
public:
    using Field = std::function<float(float, float)>;

    Ball(float x, float y);

    void update(float deltaTime, const Field& field, float width, float height);

    bool contains(float pointX, float pointY) const;
    void grab(float pointX, float pointY);
    void dragTo(float pointX, float pointY);
    void release();

    bool isHeld() const { return held; }
    float getX() const { return x; }
    float getY() const { return y; }

private:
    struct Contact {
        float overlap;
        float coverage;
        float normalX;
        float normalY;
        bool directed;
    };

    Contact probe(const Field& field, float centerX, float centerY, float margin) const;
    bool isClear(const Field& field, float centerX, float centerY) const;
    float burial(const Field& field, float centerX, float centerY) const;
    void wedge(const Field& field, float previousX, float previousY);
    void step(float duration, const Field& field, float width, float height);
    bool settle(const Field& field);
    void escape(const Field& field, float width, float height);
    void bounce(float normalX, float normalY);
    void slide(float normalX, float normalY, float duration);

    float x;
    float y;
    float velocityX;
    float velocityY;
    float targetX;
    float targetY;
    float grabOffsetX;
    float grabOffsetY;
    bool held;
};