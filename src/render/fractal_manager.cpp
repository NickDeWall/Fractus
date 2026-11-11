#include "render/fractal_manager.h"
#include "util/math_utils.h"
#include "render/shader_manager.h"
#include "render/display_modes.h"
#include <stdexcept>
#include <string>
#include <algorithm>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <string>

namespace {
    const char* COMPOSITE_FRAGMENT_SHADER = R"(
        #version 330 core
        in vec2 vTexCoord;
        uniform sampler2D tex;
        uniform vec4 color;
        out vec4 fragColor;
        void main() {
            fragColor = texture(tex, vTexCoord) * color;
        }
    )";

    const char* SCREEN_VERTEX_SHADER = R"(
        #version 330 core
        layout(location = 0) in vec2 pos;
        layout(location = 1) in vec2 texCoord;
        uniform mat4 projection;
        uniform mat4 model;
        out vec2 vTexCoord;
        void main() {
            gl_Position = projection * model * vec4(pos, 0.0, 1.0);
            vTexCoord = texCoord;
        }
    )";
}

void FractalManager::computeRenderSize(int w, int h, int& outW, int& outH) {
    const float scale = (Config::RENDER_SCALE > 0.0f) ? Config::RENDER_SCALE : 1.0f;
    outW = std::max(1, static_cast<int>(w * scale));
    outH = std::max(1, static_cast<int>(h * scale));
    const float limit = std::min(
        static_cast<float>(Config::MAX_RENDER_WIDTH) / static_cast<float>(outW),
        static_cast<float>(Config::MAX_RENDER_HEIGHT) / static_cast<float>(outH));
    if (limit < 1.0f) {
        outW = std::max(1, static_cast<int>(outW * limit));
        outH = std::max(1, static_cast<int>(outH * limit));
    }
}

FractalManager::FractalManager(int width, int height, const glm::mat4& projection)
    : width(width), height(height) {
    computeRenderSize(width, height, renderWidth, renderHeight);
    currentTexture = createTexture(renderWidth, renderHeight);
    previousTexture = createTexture(renderWidth, renderHeight);
    
    this->projection = projection;
    compositeShaderProgram = ShaderManager::createShaderProgram(SCREEN_VERTEX_SHADER, COMPOSITE_FRAGMENT_SHADER);
    const std::string fragmentShader = DisplayModes::buildFragmentShader();
    screenShaderProgram = ShaderManager::createShaderProgram(SCREEN_VERTEX_SHADER, fragmentShader.c_str());
    if (screenShaderProgram == 0) {
        throw std::runtime_error("Screen shader failed to compile; check the display mode GLSL snippets");
    }

    glGenFramebuffers(1, &fbo);
    glGenFramebuffers(1, &snapshotFbo);
    glGenVertexArrays(1, &outlineVao);
    glGenBuffers(1, &outlineVbo);
    glBindVertexArray(outlineVao);
    glBindBuffer(GL_ARRAY_BUFFER, outlineVbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, previousTexture, 0);
    glViewport(0, 0, renderWidth, renderHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f 
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

FractalManager::~FractalManager() {
    clearSnapshots();
    glDeleteProgram(screenShaderProgram);
    glDeleteProgram(compositeShaderProgram);
    glDeleteBuffers(1, &outlineVbo);
    glDeleteVertexArrays(1, &outlineVao);
    glDeleteFramebuffers(1, &snapshotFbo);
    glDeleteTextures(1, &currentTexture);
    glDeleteTextures(1, &previousTexture);
    glDeleteFramebuffers(1, &fbo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

GLuint FractalManager::createTexture(int w, int h) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

const FractalManager::OutlineCache& FractalManager::outlineGeometry(const Screen& screen) {
    OutlineCache& cache = outlines[screen.getId()];
    const std::vector<ModeEntry>& stack = screen.getStack();
    const int samples = Config::OUTLINE_SAMPLES;

    int shape = -1;
    float params[DisplayModes::MAX_PARAMS] = {};
    for (const ModeEntry& entry : stack) {
        if (entry.muted || DisplayModes::get(entry.mode).kind != ModeKind::Shape) continue;
        shape = entry.mode;
        std::copy(std::begin(entry.params), std::end(entry.params), std::begin(params));
        break;
    }

    if (cache.shape != shape || !std::equal(std::begin(params), std::end(params), std::begin(cache.params))) {
        cache.shape = shape;
        std::copy(std::begin(params), std::end(params), std::begin(cache.params));
        cache.width = 0.0f;
        cache.radii.assign(static_cast<size_t>(samples) * 2, -1.0f);

        for (int sample = 0; sample < samples; ++sample) {
            const float angle = static_cast<float>(sample) / samples * 2.0f * static_cast<float>(Config::PI);
            float crossings[2] = { -1.0f, -1.0f };
            const int found = DisplayModes::boundaryCrossings(stack, std::cos(angle), std::sin(angle), crossings, 2);
            for (int index = 0; index < found; ++index) {
                cache.radii[static_cast<size_t>(index) * samples + sample] = crossings[index];
            }
        }
    }

    const float halfWidth = std::max(screen.getWidth() * 0.5f, 1.0f);
    const float halfHeight = std::max(screen.getHeight() * 0.5f, 1.0f);
    if (cache.width == halfWidth && cache.height == halfHeight) return cache;

    cache.width = halfWidth;
    cache.height = halfHeight;
    cache.vertices.clear();
    cache.strips.clear();

    const float band = Config::OUTLINE_THICKNESS * 0.5f + 2.0f;

    for (int ring = 0; ring < 2; ++ring) {
        bool complete = true;
        for (int sample = 0; sample < samples; ++sample) {
            complete = complete && cache.radii[static_cast<size_t>(ring) * samples + sample] > 0.0f;
        }
        if (!complete) continue;

        const size_t start = cache.vertices.size() / 4;
        for (int sample = 0; sample <= samples; ++sample) {
            const int here = sample % samples;
            const int before = (here + samples - 1) % samples;
            const int after = (here + 1) % samples;

            auto point = [&](int index) {
                const float angle = static_cast<float>(index) / samples * 2.0f * static_cast<float>(Config::PI);
                const float radius = cache.radii[static_cast<size_t>(ring) * samples + index];
                return glm::vec2(std::cos(angle) * radius * halfWidth, std::sin(angle) * radius * halfHeight);
            };

            const glm::vec2 current = point(here);
            const glm::vec2 tangent = point(after) - point(before);
            const float length = std::max(std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y), 1e-4f);
            const glm::vec2 normal(tangent.y / length, -tangent.x / length);

            for (int side = 0; side < 2; ++side) {
                const glm::vec2 offset = current + normal * (side == 0 ? band : -band);
                cache.vertices.push_back(offset.x);
                cache.vertices.push_back(offset.y);
                cache.vertices.push_back(offset.x / (halfWidth * 2.0f) + 0.5f);
                cache.vertices.push_back(offset.y / (halfHeight * 2.0f) + 0.5f);
            }
        }
        cache.strips.push_back(static_cast<int>(start));
        cache.strips.push_back(static_cast<int>(cache.vertices.size() / 4 - start));
    }

    return cache;
}

void FractalManager::renderOutline(const Screen& screen, bool scaling, const glm::mat4& windowProjection) {
    const OutlineCache& cache = outlineGeometry(screen);
    if (cache.strips.empty()) return;

    const SDL_Color outlineColor = scaling ? screen.getScaleOutlineColor() : screen.getOutlineColor();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(screen.getX(), screen.getY(), 0.0f));
    model = glm::rotate(model, glm::radians(screen.getRotation()), glm::vec3(0.0f, 0.0f, 1.0f));

    glUseProgram(screenShaderProgram);
    glUniform1i(glGetUniformLocation(screenShaderProgram, "overlay"), 2);
    glUniform1f(glGetUniformLocation(screenShaderProgram, "aspect"), static_cast<float>(width) / height);
    glUniform2f(glGetUniformLocation(screenShaderProgram, "planeScale"), 1.0f, 1.0f);
    uploadStack(screen);

    glUniformMatrix4fv(glGetUniformLocation(screenShaderProgram, "projection"), 1, GL_FALSE, &windowProjection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(screenShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform4f(glGetUniformLocation(screenShaderProgram, "color"),
        outlineColor.r / 255.0f, outlineColor.g / 255.0f, outlineColor.b / 255.0f, outlineColor.a / 255.0f);

    glBindVertexArray(outlineVao);
    glBindBuffer(GL_ARRAY_BUFFER, outlineVbo);
    glBufferData(GL_ARRAY_BUFFER, cache.vertices.size() * sizeof(float), cache.vertices.data(), GL_STREAM_DRAW);
    for (size_t strip = 0; strip + 1 < cache.strips.size(); strip += 2) {
        glDrawArrays(GL_TRIANGLE_STRIP, cache.strips[strip], cache.strips[strip + 1]);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void FractalManager::uploadStack(const Screen& screen) {
    const std::vector<ModeEntry>& stack = screen.getStack();
    const int size = std::min(static_cast<int>(stack.size()), DisplayModes::MAX_STACK);

    std::vector<GLint> modes(size);
    std::vector<GLfloat> params(size * DisplayModes::MAX_PARAMS);
    for (int entry = 0; entry < size; ++entry) {
        modes[entry] = stack[entry].muted ? -1 : stack[entry].mode;
        for (int param = 0; param < DisplayModes::MAX_PARAMS; ++param) {
            params[entry * DisplayModes::MAX_PARAMS + param] = stack[entry].params[param];
        }
    }

    glUniform1i(glGetUniformLocation(screenShaderProgram, "stackSize"), size);
    if (size == 0) return;
    glUniform1iv(glGetUniformLocation(screenShaderProgram, "stackModes"), size, modes.data());
    glUniform1fv(glGetUniformLocation(screenShaderProgram, "stackParams"), static_cast<GLsizei>(params.size()), params.data());
}

void FractalManager::copyFrame(GLuint source, int sourceW, int sourceH, GLuint destination, int destinationW, int destinationH) {
    const GLenum filter = (sourceW == destinationW && sourceH == destinationH) ? GL_NEAREST : GL_LINEAR;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, snapshotFbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destination, 0);
    glBlitFramebuffer(0, 0, sourceW, sourceH, 0, 0, destinationW, destinationH, GL_COLOR_BUFFER_BIT, filter);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FractalManager::storeFrame(Frame& destination, GLuint source, int sourceW, int sourceH, int w, int h) {
    if (destination.texture == 0 || destination.width != w || destination.height != h) {
        if (destination.texture != 0) {
            glDeleteTextures(1, &destination.texture);
        }
        destination.texture = createTexture(w, h);
        destination.width = w;
        destination.height = h;
    }
    copyFrame(source, sourceW, sourceH, destination.texture, w, h);
}

void FractalManager::historySize(const Screen& screen, int& outW, int& outH) const {
    const int stepW = std::max(1, renderWidth / Config::HISTORY_SIZE_STEPS);
    const int stepH = std::max(1, renderHeight / Config::HISTORY_SIZE_STEPS);
    const float pixelsW = screen.getWidth() * static_cast<float>(renderWidth) / width;
    const float pixelsH = screen.getHeight() * static_cast<float>(renderHeight) / height;
    outW = std::clamp(static_cast<int>(std::ceil(pixelsW / stepW)) * stepW, stepW, renderWidth);
    outH = std::clamp(static_cast<int>(std::ceil(pixelsH / stepH)) * stepH, stepH, renderHeight);
}

void FractalManager::updateDelayed(const Screen& screen, double time) {
    DelayState& state = delayed[screen.getId()];
    state.used = true;

    const float delay = screen.getDelay();
    const float captureRate = static_cast<float>(MathUtils::linearInterpolate(
        Config::CAPTURE_RATE_NO_DELAY, Config::CAPTURE_RATE_MAX_DELAY, delay / Config::MAX_DELAY));
    const size_t capacity = static_cast<size_t>(std::ceil(delay * captureRate)) + 2;

    while (state.history.size() > capacity) {
        glDeleteTextures(1, &state.history.front().texture);
        state.history.pop_front();
    }

    const long long captureTick = static_cast<long long>(std::floor(time * captureRate));
    if (captureTick != state.captureTick || state.history.empty()) {
        state.captureTick = captureTick;

        Frame frame;
        if (state.history.size() >= capacity) {
            frame = state.history.front();
            state.history.pop_front();
        }

        int w, h;
        historySize(screen, w, h);
        storeFrame(frame, previousTexture, renderWidth, renderHeight, w, h);
        frame.time = time;
        state.history.push_back(frame);
    }

    const long long refreshTick = static_cast<long long>(std::floor(time * screen.getUpdateRate()));
    if (refreshTick == state.refreshTick && state.held.texture != 0) return;
    state.refreshTick = refreshTick;

    const double target = time - delay;
    const Frame* source = &state.history.front();
    for (auto it = state.history.rbegin(); it != state.history.rend(); ++it) {
        if (it->time <= target) {
            source = &*it;
            break;
        }
    }
    storeFrame(state.held, source->texture, source->width, source->height, source->width, source->height);
}

void FractalManager::deleteDelayState(DelayState& state) {
    for (auto& frame : state.history) {
        glDeleteTextures(1, &frame.texture);
    }
    state.history.clear();
    if (state.held.texture != 0) {
        glDeleteTextures(1, &state.held.texture);
        state.held.texture = 0;
    }
}

GLuint FractalManager::screenTexture(const Screen& screen) const {
    if (screen.getDelay() > 0.0f) {
        return delayed.at(screen.getId()).held.texture;
    }
    return snapshots.at(screen.getUpdateRate()).texture;
}

void FractalManager::updateSnapshots(const std::vector<Screen>& screens, double time) {
    for (auto& entry : snapshots) {
        entry.second.used = false;
    }
    for (auto& entry : delayed) {
        entry.second.used = false;
    }

    for (const auto& screen : screens) {
        if (screen.getStack().empty()) continue;

        if (screen.getDelay() > 0.0f) {
            updateDelayed(screen, time);
            continue;
        }

        const float rate = screen.getUpdateRate();
        const long long tick = static_cast<long long>(std::floor(time * rate));

        auto it = snapshots.find(rate);
        if (it == snapshots.end()) {
            GLuint texture;
            if (!spareTextures.empty()) {
                texture = spareTextures.back();
                spareTextures.pop_back();
            } else {
                texture = createTexture(renderWidth, renderHeight);
            }
            it = snapshots.emplace(rate, Snapshot{ texture, tick - 1, false }).first;
        }

        Snapshot& snapshot = it->second;
        snapshot.used = true;
        if (snapshot.tick != tick) {
            copyFrame(previousTexture, renderWidth, renderHeight, snapshot.texture, renderWidth, renderHeight);
            snapshot.tick = tick;
        }
    }

    for (auto it = snapshots.begin(); it != snapshots.end();) {
        if (it->second.used) {
            ++it;
            continue;
        }
        spareTextures.push_back(it->second.texture);
        it = snapshots.erase(it);
    }

    for (auto it = delayed.begin(); it != delayed.end();) {
        if (it->second.used) {
            ++it;
            continue;
        }
        deleteDelayState(it->second);
        it = delayed.erase(it);
    }
}

void FractalManager::clearSnapshots() {
    for (auto& entry : snapshots) {
        glDeleteTextures(1, &entry.second.texture);
    }
    snapshots.clear();
    if (!spareTextures.empty()) {
        glDeleteTextures(static_cast<GLsizei>(spareTextures.size()), spareTextures.data());
    }
    spareTextures.clear();
    for (auto& entry : delayed) {
        deleteDelayState(entry.second);
    }
    delayed.clear();
}

GLuint FractalManager::processFrame(const std::vector<Screen>& screens, int frameCounter, double time) {
    updateSnapshots(screens, time);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currentTexture, 0);
    
    glViewport(0, 0, renderWidth, renderHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glm::mat4 offscreenProjection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    
    for (const auto& screen : screens) {
        glUseProgram(screenShaderProgram);
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(screen.getX(), height - screen.getY(), 0.0f));
        model = glm::rotate(model, glm::radians(-screen.getRotation() + 180), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(screen.getWidth()/2.0f, -screen.getHeight()/2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(-(float)screen.getWidth(), (float)screen.getHeight(), 1.0f));
        
        GLint projLoc = glGetUniformLocation(screenShaderProgram, "projection");
        GLint modelLoc = glGetUniformLocation(screenShaderProgram, "model");
        GLint texLoc = glGetUniformLocation(screenShaderProgram, "tex");
        GLint colorLoc = glGetUniformLocation(screenShaderProgram, "color");
        
        if (!screen.getStack().empty()) {
            glUniform1f(glGetUniformLocation(screenShaderProgram, "aspect"), static_cast<float>(width) / height);
            glUniform1i(glGetUniformLocation(screenShaderProgram, "overlay"), 0);
            glUniform2f(glGetUniformLocation(screenShaderProgram, "planeScale"), 1.0f, 1.0f);
            uploadStack(screen);

            if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &offscreenProjection[0][0]);
            if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            if (colorLoc != -1) glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, screenTexture(screen));
            if (texLoc != -1) glUniform1i(texLoc, 0);

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);
        }
        glUseProgram(0);
        
        float r, g, b, alpha;
        screen.getColorF(r, g, b, alpha);
        
        glUseProgram(screenShaderProgram);

        projLoc = glGetUniformLocation(screenShaderProgram, "projection");
        modelLoc = glGetUniformLocation(screenShaderProgram, "model");
        colorLoc = glGetUniformLocation(screenShaderProgram, "color");

        glUniform1i(glGetUniformLocation(screenShaderProgram, "overlay"), 1);
        glUniform1f(glGetUniformLocation(screenShaderProgram, "aspect"), static_cast<float>(width) / height);
        glUniform2f(glGetUniformLocation(screenShaderProgram, "planeScale"), 1.0f, 1.0f);
        uploadStack(screen);

        if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &offscreenProjection[0][0]);
        if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        if (colorLoc != -1) glUniform4f(colorLoc, r * alpha, g * alpha, b * alpha, alpha);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    std::swap(currentTexture, previousTexture);
    
    return previousTexture;
}

void FractalManager::resize(int newWidth, int newHeight, const glm::mat4& newProjection) {
    projection = newProjection;

    if (newWidth <= 0 || newHeight <= 0) return;

    int newRenderWidth = 0, newRenderHeight = 0;
    computeRenderSize(newWidth, newHeight, newRenderWidth, newRenderHeight);

    const bool renderSizeChanged =
        (newRenderWidth != renderWidth || newRenderHeight != renderHeight);
    width = newWidth;
    height = newHeight;
    if (!renderSizeChanged) return;

    clearSnapshots();

    GLuint newCurrent = createTexture(newRenderWidth, newRenderHeight);
    GLuint newPrevious = createTexture(newRenderWidth, newRenderHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, newRenderWidth, newRenderHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newCurrent, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newPrevious, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    glDisable(GL_BLEND);

    glUseProgram(compositeShaderProgram);

    glm::mat4 blitProjection = glm::ortho(0.0f, static_cast<float>(newRenderWidth),
                                          static_cast<float>(newRenderHeight), 0.0f, -1.0f, 1.0f);
    glm::mat4 model = glm::scale(glm::mat4(1.0f),
                                 glm::vec3(static_cast<float>(newRenderWidth),
                                           static_cast<float>(newRenderHeight), 1.0f));

    GLint projLoc = glGetUniformLocation(compositeShaderProgram, "projection");
    GLint modelLoc = glGetUniformLocation(compositeShaderProgram, "model");
    GLint texLoc = glGetUniformLocation(compositeShaderProgram, "tex");
    GLint colorLoc = glGetUniformLocation(compositeShaderProgram, "color");

    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &blitProjection[0][0]);
    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    if (colorLoc != -1) glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, previousTexture);
    if (texLoc != -1) glUniform1i(texLoc, 0);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (blendWasEnabled) glEnable(GL_BLEND);

    glDeleteTextures(1, &currentTexture);
    glDeleteTextures(1, &previousTexture);

    currentTexture = newCurrent;
    previousTexture = newPrevious;
    renderWidth = newRenderWidth;
    renderHeight = newRenderHeight;
}

void FractalManager::renderCurrentFrame() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(compositeShaderProgram);
    
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    GLint projLoc = glGetUniformLocation(compositeShaderProgram, "projection");
    GLint modelLoc = glGetUniformLocation(compositeShaderProgram, "model");
    GLint texLoc = glGetUniformLocation(compositeShaderProgram, "tex");
    GLint colorLoc = glGetUniformLocation(compositeShaderProgram, "color");
    
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    if (colorLoc != -1) glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, previousTexture);
    if (texLoc != -1) glUniform1i(texLoc, 0);
    
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
}