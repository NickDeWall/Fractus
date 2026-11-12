#include "scene/ball.h"
#include "core/config.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {
    const float STEP = 1.0f / 600.0f;
    const Ball::Motion STILL = [](float, float, float, float) { return 0.0f; };

    void run(Ball& ball, const Ball::Field& field, float width, float height, float seconds) {
        const int frames = static_cast<int>(seconds / STEP);
        for (int i = 0; i < frames; ++i) {
            ball.update(STEP, field, STILL, width, height);
        }
    }

    float ramp(float depth) {
        return std::clamp(depth * 0.25f, -0.5f, 0.5f);
    }
}

int main() {
    const float radius = Config::BALL_RADIUS;
    const Ball::Field empty = [](float, float) { return -0.5f; };
    const Ball::Field ground = [](float, float y) { return ramp(y - 300.0f); };
    const Ball::Field slope = [](float x, float y) { return ramp((y - 300.0f - (x - 400.0f) * 0.5f) * 0.894f); };

    Ball dropped(400.0f, 100.0f);
    run(dropped, empty, 5000.0f, 500.0f, 5.0f);
    assert(std::fabs(dropped.getY() + radius - 500.0f) < 0.5f);

    Ball landed(400.0f, 100.0f);
    run(landed, ground, 5000.0f, 1000.0f, 5.0f);
    const float restY = landed.getY();
    assert(std::fabs(restY + radius - 300.0f) < 1.0f);
    run(landed, ground, 5000.0f, 1000.0f, 0.5f);
    assert(std::fabs(landed.getY() - restY) < 0.1f);
    assert(std::fabs(landed.getX() - 400.0f) < 0.1f);

    Ball rolling(400.0f, 250.0f);
    run(rolling, slope, 5000.0f, 5000.0f, 1.0f);
    assert(rolling.getX() > 450.0f);
    assert(slope(rolling.getX(), rolling.getY()) < 0.0f);

    Ball buried(400.0f, 400.0f);
    run(buried, ground, 5000.0f, 1000.0f, 1.0f);
    assert(buried.getY() + radius < 301.0f);

    const Ball::Field notch = [](float x, float y) { return ramp((y - 300.0f + std::fabs(x - 400.0f)) * 0.707f); };
    Ball nested(410.0f, 100.0f);
    run(nested, notch, 5000.0f, 5000.0f, 5.0f);
    const float nestedX = nested.getX();
    const float nestedY = nested.getY();
    assert(std::fabs(nestedX - 400.0f) < 1.5f);
    assert(std::fabs(nestedY - (300.0f - radius * 1.414f)) < 1.5f);
    run(nested, notch, 5000.0f, 5000.0f, 0.5f);
    assert(std::fabs(nested.getX() - nestedX) < 0.2f && std::fabs(nested.getY() - nestedY) < 0.2f);

    const Ball::Field pinch = [](float x, float y) { return y < 300.0f ? ramp(std::fabs(x - 400.0f) - 14.0f) : -0.5f; };
    Ball pinched(400.0f, 100.0f);
    run(pinched, pinch, 5000.0f, 5000.0f, 1.0f);
    assert(std::fabs(pinched.getX() - 400.0f) < 1.0f && std::fabs(pinched.getY() - 100.0f) < 1.0f);

    const Ball::Field crush = [](float x, float y) { return y < 300.0f ? ramp(std::fabs(x - 400.0f) - 8.0f) : -0.5f; };
    Ball crushed(400.0f, 100.0f);
    run(crushed, crush, 5000.0f, 1000.0f, 0.1f);
    assert(crushed.getY() - radius > 299.0f);

    Ball thrown(400.0f, 100.0f);
    thrown.grab(400.0f, 100.0f);
    for (int i = 0; i < 120; ++i) {
        thrown.dragTo(400.0f + i, 100.0f);
        thrown.update(STEP, empty, STILL, 5000.0f, 5000.0f);
    }
    thrown.release();
    const float releaseX = thrown.getX();
    run(thrown, empty, 5000.0f, 5000.0f, 0.1f);
    assert(thrown.getX() > releaseX + 30.0f);

    run(thrown, empty, 600.0f, 500.0f, 5.0f);
    assert(thrown.getX() <= 600.0f - radius);
    assert(thrown.getX() >= radius);

    float wallX = 300.0f;
    const Ball::Field wall = [&wallX](float x, float) { return ramp(wallX - x); };
    const Ball::Motion advancing = [](float, float, float normalX, float) { return 600.0f * normalX; };
    Ball shoved(350.0f, 100.0f);
    Ball whacked(350.0f, 100.0f);
    for (int i = 0; i < 300; ++i) {
        wallX += 600.0f * STEP;
        shoved.update(STEP, wall, STILL, 5000.0f, 5000.0f);
        whacked.update(STEP, wall, advancing, 5000.0f, 5000.0f);
    }
    assert(shoved.getX() < wallX + radius + 4.0f);
    assert(whacked.getX() > wallX + radius + 60.0f);

    std::puts("ball_check passed");
    return 0;
}