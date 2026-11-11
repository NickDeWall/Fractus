#pragma once
#include <GL/glew.h>
#include <algorithm>
#include <vector>
#include "core/config.h"

class CollisionField {
public:
    CollisionField();
    ~CollisionField();

    CollisionField(const CollisionField&) = delete;
    CollisionField& operator=(const CollisionField&) = delete;

    void update(GLuint canvas, int newWindowWidth, int newWindowHeight, float deltaTime);
    void renderOutline();

    float sample(float x, float y) const;
    float depth(float x, float y) const;
    bool solid(float x, float y) const;

    static float sampleGrid(const unsigned char* data, int width, int height, float u, float v);

    float floorLevel = Config::COLLISION_FLOOR;
    float fraction = Config::COLLISION_PEAK_FRACTION;
    float neighborhood = Config::COLLISION_NEIGHBORHOOD;
    float smoothness = Config::COLLISION_SMOOTHNESS;
    float threshold = Config::COLLISION_THRESHOLD;
    float timeSmoothing = Config::COLLISION_TIME_SMOOTHING;
    bool visible = false;

private:
    struct Readback {
        GLuint buffer = 0;
        GLsync sync = nullptr;
        int width = 0;
        int height = 0;
        unsigned long long order = 0;
    };

    void resize(int width, int height);
    void blur(GLuint* pair, float radius, int lod);
    void steady(float rate);
    void collect();
    void request();

    GLuint solidityProgram;
    GLuint blurProgram;
    GLuint blendProgram;
    GLuint outlineProgram;
    GLint floorLocation;
    GLint fractionLocation;
    GLint measureLocation;
    GLint axisLocation;
    GLint radiusLocation;
    GLint lodLocation;
    GLint rateLocation;
    GLint thresholdLocation;

    GLuint fbo;
    GLuint vao;
    GLuint textures[2];
    GLuint levels[2];
    GLuint history[2];
    bool historyValid;
    int fieldWidth;
    int fieldHeight;
    int windowWidth;
    int windowHeight;

    Readback readbacks[Config::COLLISION_READBACK_SLOTS];
    unsigned long long issued;
    unsigned long long received;

    std::vector<unsigned char> data;
    int dataWidth;
    int dataHeight;
};

inline float CollisionField::sampleGrid(const unsigned char* data, int width, int height, float u, float v) {
    if (!data || width <= 0 || height <= 0) return 0.0f;

    const float x = std::clamp(u * width - 0.5f, 0.0f, static_cast<float>(width - 1));
    const float y = std::clamp(v * height - 0.5f, 0.0f, static_cast<float>(height - 1));
    const int left = static_cast<int>(x);
    const int top = static_cast<int>(y);
    const int right = std::min(left + 1, width - 1);
    const int bottom = std::min(top + 1, height - 1);
    const float blendX = x - left;
    const float blendY = y - top;

    const float upper = data[top * width + left] * (1.0f - blendX) + data[top * width + right] * blendX;
    const float lower = data[bottom * width + left] * (1.0f - blendX) + data[bottom * width + right] * blendX;
    return (upper * (1.0f - blendY) + lower * blendY) / 255.0f;
}