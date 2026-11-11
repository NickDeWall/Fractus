#include "scene/ball.h"
#include "core/config.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
    std::vector<SDL_FPoint> makeRim() {
        std::vector<SDL_FPoint> rim(Config::BALL_SAMPLES);
        for (int i = 0; i < Config::BALL_SAMPLES; ++i) {
            const float angle = i * 6.28318530718f / Config::BALL_SAMPLES;
            rim[i] = { std::cos(angle), std::sin(angle) };
        }
        return rim;
    }

    const std::vector<SDL_FPoint> RIM = makeRim();
}

Ball::Ball(float x, float y)
    : x(x), y(y), velocityX(0.0f), velocityY(0.0f), targetX(x), targetY(y),
      grabOffsetX(0.0f), grabOffsetY(0.0f), held(false) {}

bool Ball::contains(float pointX, float pointY) const {
    return std::hypot(pointX - x, pointY - y) <= Config::BALL_RADIUS;
}

void Ball::grab(float pointX, float pointY) {
    held = true;
    grabOffsetX = x - pointX;
    grabOffsetY = y - pointY;
    targetX = x;
    targetY = y;
    velocityX = 0.0f;
    velocityY = 0.0f;
}

void Ball::dragTo(float pointX, float pointY) {
    targetX = pointX + grabOffsetX;
    targetY = pointY + grabOffsetY;
}

void Ball::release() {
    held = false;
}

void Ball::update(float deltaTime, const Field& field, float width, float height) {
    if (deltaTime <= 0.0f) return;

    if (held) {
        const float blend = std::min(1.0f, deltaTime / Config::BALL_THROW_SMOOTHING);
        velocityX += ((targetX - x) / deltaTime - velocityX) * blend;
        velocityY += ((targetY - y) / deltaTime - velocityY) * blend;
        x = targetX;
        y = targetY;
        return;
    }

    const float travel = std::hypot(velocityX, velocityY) * deltaTime;
    const int steps = std::clamp(static_cast<int>(std::ceil(travel / (Config::BALL_RADIUS * 0.5f))), 1, Config::BALL_MAX_SUBSTEPS);
    for (int i = 0; i < steps; ++i) {
        step(deltaTime / steps, field, width, height);
    }
}

void Ball::step(float duration, const Field& field, float width, float height) {
    velocityY += Config::BALL_GRAVITY * duration;

    const float speed = std::hypot(velocityX, velocityY);
    if (speed > Config::BALL_MAX_SPEED) {
        velocityX *= Config::BALL_MAX_SPEED / speed;
        velocityY *= Config::BALL_MAX_SPEED / speed;
    }

    const float previousX = x;
    const float previousY = y;
    x += velocityX * duration;
    y += velocityY * duration;

    const bool freed = settle(field);

    if (x < Config::BALL_RADIUS) {
        x = Config::BALL_RADIUS;
        bounce(1.0f, 0.0f);
    } else if (x > width - Config::BALL_RADIUS) {
        x = width - Config::BALL_RADIUS;
        bounce(-1.0f, 0.0f);
    }

    const float rest = height - Config::BALL_RADIUS;
    if (y > rest) {
        y = rest;
        bounce(0.0f, -1.0f);
    }

    if (!freed) {
        velocityX = 0.0f;
        velocityY = 0.0f;
        wedge(field, previousX, previousY);
        if (probe(field, x, y, 0.0f).coverage > Config::BALL_SQUEEZE_LIMIT) {
            escape(field, width, height);
        }
        return;
    }

    const Contact touch = probe(field, x, y, Config::BALL_SKIN);
    if (touch.overlap > 0.0f && touch.directed) {
        slide(touch.normalX, touch.normalY, duration);
    } else if (y >= rest - 0.5f) {
        slide(0.0f, -1.0f, duration);
    }
}

Ball::Contact Ball::probe(const Field& field, float centerX, float centerY, float margin) const {
    Contact contact = { 0.0f, 0.0f, 0.0f, -1.0f, false };
    float sumX = 0.0f;
    float sumY = 0.0f;
    int buried = 0;

    for (const SDL_FPoint& direction : RIM) {
        const float depth = field(centerX + direction.x * Config::BALL_RADIUS, centerY + direction.y * Config::BALL_RADIUS) + margin;
        if (depth <= 0.0f) continue;

        ++buried;
        contact.overlap += depth;
        sumX -= direction.x * depth;
        sumY -= direction.y * depth;
    }

    contact.coverage = static_cast<float>(buried) / Config::BALL_SAMPLES;

    const float length = std::hypot(sumX, sumY);
    if (length > contact.overlap * Config::BALL_PINCH_RATIO) {
        contact.normalX = sumX / length;
        contact.normalY = sumY / length;
        contact.directed = true;
    }
    return contact;
}

bool Ball::isClear(const Field& field, float centerX, float centerY) const {
    for (const SDL_FPoint& direction : RIM) {
        if (field(centerX + direction.x * Config::BALL_RADIUS, centerY + direction.y * Config::BALL_RADIUS) > 0.0f) return false;
    }
    return true;
}

float Ball::burial(const Field& field, float centerX, float centerY) const {
    float total = std::max(field(centerX, centerY), 0.0f);
    for (int ring = 1; ring <= Config::BALL_BURIAL_RINGS; ++ring) {
        const float reach = Config::BALL_RADIUS * ring / Config::BALL_BURIAL_RINGS;
        for (const SDL_FPoint& direction : RIM) {
            total += std::max(field(centerX + direction.x * reach, centerY + direction.y * reach), 0.0f) * ring;
        }
    }
    return total;
}

void Ball::wedge(const Field& field, float previousX, float previousY) {
    float bestX = previousX;
    float bestY = previousY;
    float least = burial(field, previousX, previousY);

    const auto consider = [&](float centerX, float centerY) {
        const float amount = burial(field, centerX, centerY);
        if (amount < least * Config::BALL_WEDGE_GAIN) {
            least = amount;
            bestX = centerX;
            bestY = centerY;
        }
    };

    consider(x, y);
    for (int i = 0; i < Config::BALL_SAMPLES; i += Config::BALL_SAMPLES / 8) {
        consider(previousX + RIM[i].x * Config::BALL_MAX_PUSH, previousY + RIM[i].y * Config::BALL_MAX_PUSH);
    }

    x = bestX;
    y = bestY;
}

bool Ball::settle(const Field& field) {
    for (int pass = 0; pass < Config::BALL_RESOLVE_PASSES; ++pass) {
        const Contact contact = probe(field, x, y, 0.0f);
        if (contact.overlap <= 0.0f) return true;
        if (!contact.directed) return false;

        float blocked = 0.0f;
        float clear = 0.0f;
        float best = 0.0f;
        float bestOverlap = contact.overlap;

        for (float reach = Config::BALL_MAX_PUSH * 0.125f; reach <= Config::BALL_MAX_PUSH; reach *= 2.0f) {
            const float overlap = probe(field, x + contact.normalX * reach, y + contact.normalY * reach, 0.0f).overlap;
            if (overlap <= 0.0f) {
                clear = reach;
                break;
            }
            blocked = reach;
            if (overlap < bestOverlap) {
                bestOverlap = overlap;
                best = reach;
            }
        }

        if (clear > 0.0f) {
            for (int i = 0; i < 8; ++i) {
                const float middle = (blocked + clear) * 0.5f;
                if (isClear(field, x + contact.normalX * middle, y + contact.normalY * middle)) {
                    clear = middle;
                } else {
                    blocked = middle;
                }
            }
            best = clear;
        }

        if (best <= 0.0f) return false;

        x += contact.normalX * best;
        y += contact.normalY * best;
        bounce(contact.normalX, contact.normalY);
    }

    return isClear(field, x, y);
}

void Ball::escape(const Field& field, float width, float height) {
    const int count = Config::BALL_SAMPLES;
    const int up = count * 3 / 4;
    const float stride = Config::BALL_RADIUS * 0.5f;

    for (float distance = stride; distance <= Config::BALL_ESCAPE_RANGE; distance += stride) {
        for (int i = 0; i < count; ++i) {
            const int offset = (i + 1) / 2 * (i % 2 == 0 ? -1 : 1);
            const SDL_FPoint& direction = RIM[(up + offset + count) % count];
            const float centerX = x + direction.x * distance;
            const float centerY = y + direction.y * distance;

            if (centerX < Config::BALL_RADIUS || centerX > width - Config::BALL_RADIUS) continue;
            if (centerY < Config::BALL_RADIUS || centerY > height - Config::BALL_RADIUS) continue;
            if (!isClear(field, centerX, centerY)) continue;

            x = centerX;
            y = centerY;
            return;
        }
    }
}

void Ball::bounce(float normalX, float normalY) {
    const float inward = velocityX * normalX + velocityY * normalY;
    if (inward >= 0.0f) return;

    const float restitution = -inward > Config::BALL_REST_SPEED ? Config::BALL_BOUNCE : 0.0f;
    velocityX -= (1.0f + restitution) * inward * normalX;
    velocityY -= (1.0f + restitution) * inward * normalY;
}

void Ball::slide(float normalX, float normalY, float duration) {
    const float along = velocityX * normalX + velocityY * normalY;
    const float keep = std::max(0.0f, 1.0f - Config::BALL_FRICTION * duration);
    velocityX = (velocityX - along * normalX) * keep + along * normalX;
    velocityY = (velocityY - along * normalY) * keep + along * normalY;
}