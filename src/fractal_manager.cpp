#include "fractal_manager.h"
#include "math_utils.h"
#include "shader_manager.h"
#include <algorithm>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <string>

namespace {
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

    const char* SCREEN_FRAGMENT_SHADER = R"(
        #version 330 core
        const int LOG_POLAR = 2;
        const float TAU = 6.28318530718;
        in vec2 vTexCoord;
        uniform sampler2D tex;
        uniform vec4 color;
        uniform int mode;
        uniform float aspect;
        uniform float minRadius;
        out vec4 fragColor;
        void main() {
            vec2 uv = vTexCoord;
            if (mode == LOG_POLAR) {
                float maxRadius = 0.5 * min(aspect, 1.0);
                float lowRadius = maxRadius * minRadius;
                float radius = lowRadius * exp(vTexCoord.x * log(maxRadius / lowRadius));
                float angle = vTexCoord.y * TAU;
                uv = vec2(0.5 + radius * cos(angle) / aspect, 0.5 + radius * sin(angle));
            }
            fragColor = texture(tex, uv) * color;
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

FractalManager::FractalManager(int width, int height, GLuint textureShader, GLuint colorShader, const glm::mat4& projection)
    : width(width), height(height), textureShaderProgram(textureShader), colorShaderProgram(colorShader) {
    computeRenderSize(width, height, renderWidth, renderHeight);
    currentTexture = createTexture(renderWidth, renderHeight);
    previousTexture = createTexture(renderWidth, renderHeight);
    
    this->projection = projection;
    screenShaderProgram = ShaderManager::createShaderProgram(SCREEN_VERTEX_SHADER, SCREEN_FRAGMENT_SHADER);

    glGenFramebuffers(1, &fbo);
    glGenFramebuffers(1, &snapshotFbo);
    
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
        if (screen.getDisplayMode() == DisplayMode::Prop) continue;

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
        
        if (screen.getDisplayMode() != DisplayMode::Prop) {
            glUniform1i(glGetUniformLocation(screenShaderProgram, "mode"), static_cast<int>(screen.getDisplayMode()));
            glUniform1f(glGetUniformLocation(screenShaderProgram, "aspect"), static_cast<float>(width) / height);
            glUniform1f(glGetUniformLocation(screenShaderProgram, "minRadius"), Config::LOG_POLAR_MIN_RADIUS);

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
        
        glUseProgram(colorShaderProgram);
        
        projLoc = glGetUniformLocation(colorShaderProgram, "projection");
        modelLoc = glGetUniformLocation(colorShaderProgram, "model");
        colorLoc = glGetUniformLocation(colorShaderProgram, "color");
        
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

    glUseProgram(textureShaderProgram);

    glm::mat4 blitProjection = glm::ortho(0.0f, static_cast<float>(newRenderWidth),
                                          static_cast<float>(newRenderHeight), 0.0f, -1.0f, 1.0f);
    glm::mat4 model = glm::scale(glm::mat4(1.0f),
                                 glm::vec3(static_cast<float>(newRenderWidth),
                                           static_cast<float>(newRenderHeight), 1.0f));

    GLint projLoc = glGetUniformLocation(textureShaderProgram, "projection");
    GLint modelLoc = glGetUniformLocation(textureShaderProgram, "model");
    GLint texLoc = glGetUniformLocation(textureShaderProgram, "tex");
    GLint colorLoc = glGetUniformLocation(textureShaderProgram, "color");

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

    glUseProgram(textureShaderProgram);
    
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    GLint projLoc = glGetUniformLocation(textureShaderProgram, "projection");
    GLint modelLoc = glGetUniformLocation(textureShaderProgram, "model");
    GLint texLoc = glGetUniformLocation(textureShaderProgram, "tex");
    GLint colorLoc = glGetUniformLocation(textureShaderProgram, "color");
    
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

namespace OtherRenders {
    void renderSelectionOutline(Screen* selected, bool scalingMode, int tempWidth, int tempHeight, GLuint colorShaderProgram, const glm::mat4& projection, GLuint vao) {
        if (!selected) return;

        int width = scalingMode ? tempWidth : selected->getWidth();
        int height = scalingMode ? tempHeight : selected->getHeight();
        float centerX = selected->getX();
        float centerY = selected->getY();
        float rotation = selected->getRotation();

        SDL_Color outlineColor = scalingMode ? selected->getScaleOutlineColor() : selected->getOutlineColor();
        
        glUseProgram(colorShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(colorShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniform4f(glGetUniformLocation(colorShaderProgram, "color"), outlineColor.r / 255.0f, outlineColor.g / 255.0f, outlineColor.b / 255.0f, outlineColor.a / 255.0f);
        glBindVertexArray(vao);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centerX, centerY, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-(width + 2*Config::OUTLINE_THICKNESS)/2, height/2, 0.0f));
        model = glm::scale(model, glm::vec3(width + 2*Config::OUTLINE_THICKNESS, Config::OUTLINE_THICKNESS, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(colorShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centerX, centerY, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-(width + 2*Config::OUTLINE_THICKNESS)/2, -height/2 - Config::OUTLINE_THICKNESS, 0.0f));
        model = glm::scale(model, glm::vec3(width + 2*Config::OUTLINE_THICKNESS, Config::OUTLINE_THICKNESS, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(colorShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centerX, centerY, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-width/2 - Config::OUTLINE_THICKNESS, -height/2, 0.0f));
        model = glm::scale(model, glm::vec3(Config::OUTLINE_THICKNESS, height, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(colorShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centerX, centerY, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(width/2, -height/2, 0.0f));
        model = glm::scale(model, glm::vec3(Config::OUTLINE_THICKNESS, height, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(colorShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void initGL(int width, int height, GLuint& vao, GLuint& vbo) {
        glViewport(0, 0, width, height);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

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
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void cleanupGL(GLuint& vao, GLuint& vbo) {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    bool initFPSTexture(GLuint& fpsTexture) {
        glGenTextures(1, &fpsTexture);
        glBindTexture(GL_TEXTURE_2D, fpsTexture);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        unsigned char emptyPixels[4] = {0, 0, 0, 0};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, emptyPixels);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    bool updateFPSTexture(int fps, TTF_Font* font, GLuint& fpsTexture, int& outWidth, int& outHeight) {
        if (!font) return false;
        
        std::string fpsText = std::to_string(fps);
        SDL_Surface* surface = TTF_RenderText_Blended(font, fpsText.c_str(), Config::FPS_TEXT_COLOR);
        if (!surface) return false;
        
        SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        
        if (!rgbaSurface) return false;
        
        glBindTexture(GL_TEXTURE_2D, fpsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgbaSurface->w, rgbaSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaSurface->pixels);
        
        outWidth = rgbaSurface->w;
        outHeight = rgbaSurface->h;
        SDL_FreeSurface(rgbaSurface);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    void renderFPS(int screenWidth, int screenHeight, GLuint fpsTexture, int fpsWidth, int fpsHeight, GLuint textureShaderProgram, const glm::mat4& projection, GLuint vao) {
        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLint srcBlend, dstBlend;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcBlend);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstBlend);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
                
        glUseProgram(textureShaderProgram);
        
        float x = static_cast<float>(screenWidth - fpsWidth);
        float y = 0.0f;
                
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(fpsWidth, fpsHeight, 1.0f));
        
        glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform4f(glGetUniformLocation(textureShaderProgram, "color"), 1.0f, 1.0f, 1.0f, 1.0f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fpsTexture);
        glUniform1i(glGetUniformLocation(textureShaderProgram, "tex"), 0);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        
        if (!blendEnabled) {
            glDisable(GL_BLEND);
        } else {
            glBlendFunc(srcBlend, dstBlend);
        }
    }

    bool initDebugTexture(GLuint& debugTexture) {
        glGenTextures(1, &debugTexture);
        glBindTexture(GL_TEXTURE_2D, debugTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        unsigned char emptyPixels[4] = {0, 0, 0, 0};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, emptyPixels);

        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    bool updateDebugTexture(const std::string& debugText, TTF_Font* font, GLuint& debugTexture, int& outWidth, int& outHeight) {
        if (!font || debugText.empty()) {
            outWidth = 0;
            outHeight = 0;
            return false;
        }

        SDL_Surface* surface = TTF_RenderText_Blended(font, debugText.c_str(), Config::FPS_TEXT_COLOR);
        if (!surface) return false;

        SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);

        if (!rgbaSurface) return false;

        glBindTexture(GL_TEXTURE_2D, debugTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgbaSurface->w, rgbaSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaSurface->pixels);

        outWidth = rgbaSurface->w;
        outHeight = rgbaSurface->h;
        SDL_FreeSurface(rgbaSurface);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    void renderDebugText(int screenWidth, int screenHeight, GLuint debugTexture, int debugWidth, int debugHeight, GLuint textureShaderProgram, const glm::mat4& projection, GLuint vao) {
        if (debugWidth == 0 || debugHeight == 0) return;

        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLint srcBlend, dstBlend;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcBlend);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstBlend);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(textureShaderProgram);

        float x = static_cast<float>(screenWidth / 2);
        float y = static_cast<float>(screenHeight - debugHeight);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(debugWidth, debugHeight, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform4f(glGetUniformLocation(textureShaderProgram, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, debugTexture);
        glUniform1i(glGetUniformLocation(textureShaderProgram, "tex"), 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);

        if (!blendEnabled) {
            glDisable(GL_BLEND);
        } else {
            glBlendFunc(srcBlend, dstBlend);
        }
    }
}