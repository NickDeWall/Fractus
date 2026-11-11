#include "render/collision_field.h"
#include "render/shader_manager.h"
#include <cmath>

namespace {
    const char* VERTEX_SHADER = R"(
        #version 330 core
        uniform float flip;
        out vec2 uv;
        void main() {
            uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
            gl_Position = vec4(uv.x * 2.0 - 1.0, (uv.y * 2.0 - 1.0) * flip, 0.0, 1.0);
        }
    )";

    const char* SOLIDITY_FRAGMENT_SHADER = R"(
        #version 330 core
        in vec2 uv;
        uniform sampler2D canvas;
        uniform sampler2D surround;
        uniform float floorLevel;
        uniform float fraction;
        uniform float surroundLod;
        uniform float peakPower;
        uniform bool measure;
        out float solidity;
        float cover(vec2 at) {
            float level = texture(canvas, at).a;
            if (measure) return pow(level, peakPower);
            float peak = pow(textureLod(surround, at, surroundLod).r, 1.0 / peakPower);
            return step(max(floorLevel, fraction * peak), level);
        }
        void main() {
            vec2 reach = fwidth(uv) * 0.25;
            float total = cover(uv + vec2(-reach.x, -reach.y));
            total += cover(uv + vec2(reach.x, -reach.y));
            total += cover(uv + vec2(-reach.x, reach.y));
            total += cover(uv + vec2(reach.x, reach.y));
            solidity = total * 0.25;
        }
    )";

    const char* BLUR_FRAGMENT_SHADER = R"(
        #version 330 core
        in vec2 uv;
        uniform sampler2D field;
        uniform vec2 axis;
        uniform float radius;
        uniform float lod;
        out float blurred;
        void main() {
            float sigma = radius * 0.5;
            float falloff = -0.5 / (sigma * sigma);
            float total = textureLod(field, uv, lod).r;
            float weights = 1.0;
            for (int tap = 1; float(tap) <= radius; tap += 2) {
                float inner = exp(falloff * float(tap * tap));
                float outer = exp(falloff * float((tap + 1) * (tap + 1)));
                float weight = inner + outer;
                vec2 reach = axis * ((float(tap) * inner + float(tap + 1) * outer) / weight);
                total += weight * (textureLod(field, uv + reach, lod).r + textureLod(field, uv - reach, lod).r);
                weights += 2.0 * weight;
            }
            blurred = total / weights;
        }
    )";

    const char* BLEND_FRAGMENT_SHADER = R"(
        #version 330 core
        in vec2 uv;
        uniform sampler2D previous;
        uniform sampler2D current;
        uniform float rate;
        out float blended;
        void main() {
            blended = mix(texture(previous, uv).r, texture(current, uv).r, rate);
        }
    )";

    const char* OUTLINE_FRAGMENT_SHADER = R"(
        #version 330 core
        in vec2 uv;
        uniform sampler2D field;
        uniform vec3 tint;
        uniform float edgeAlpha;
        uniform float threshold;
        out vec4 fragColor;
        void main() {
            float value = texture(field, uv).r;
            float pixels = (value - threshold) / max(fwidth(value), 1e-5);
            fragColor = vec4(tint, clamp(1.5 - abs(pixels), 0.0, 1.0) * edgeAlpha);
        }
    )";

    GLuint createFieldTexture(int width, int height, GLint format, bool mipmapped) {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        if (mipmapped) glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmapped ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }
}

CollisionField::CollisionField()
    : textures{ 0, 0 }, levels{ 0, 0 }, history{ 0, 0 }, historyValid(false), fieldWidth(0), fieldHeight(0), windowWidth(0), windowHeight(0),
      issued(0), received(0), dataWidth(0), dataHeight(0) {
    solidityProgram = ShaderManager::createShaderProgram(VERTEX_SHADER, SOLIDITY_FRAGMENT_SHADER);
    blurProgram = ShaderManager::createShaderProgram(VERTEX_SHADER, BLUR_FRAGMENT_SHADER);
    blendProgram = ShaderManager::createShaderProgram(VERTEX_SHADER, BLEND_FRAGMENT_SHADER);
    outlineProgram = ShaderManager::createShaderProgram(VERTEX_SHADER, OUTLINE_FRAGMENT_SHADER);

    glUseProgram(solidityProgram);
    glUniform1f(glGetUniformLocation(solidityProgram, "flip"), 1.0f);
    glUniform1i(glGetUniformLocation(solidityProgram, "canvas"), 0);
    glUniform1i(glGetUniformLocation(solidityProgram, "surround"), 1);
    glUniform1f(glGetUniformLocation(solidityProgram, "surroundLod"), static_cast<float>(Config::COLLISION_SURROUND_LOD));
    glUniform1f(glGetUniformLocation(solidityProgram, "peakPower"), Config::COLLISION_PEAK_POWER);
    floorLocation = glGetUniformLocation(solidityProgram, "floorLevel");
    fractionLocation = glGetUniformLocation(solidityProgram, "fraction");
    measureLocation = glGetUniformLocation(solidityProgram, "measure");

    glUseProgram(blurProgram);
    glUniform1f(glGetUniformLocation(blurProgram, "flip"), 1.0f);
    glUniform1i(glGetUniformLocation(blurProgram, "field"), 0);
    axisLocation = glGetUniformLocation(blurProgram, "axis");
    radiusLocation = glGetUniformLocation(blurProgram, "radius");
    lodLocation = glGetUniformLocation(blurProgram, "lod");

    glUseProgram(blendProgram);
    glUniform1f(glGetUniformLocation(blendProgram, "flip"), 1.0f);
    glUniform1i(glGetUniformLocation(blendProgram, "previous"), 0);
    glUniform1i(glGetUniformLocation(blendProgram, "current"), 1);
    rateLocation = glGetUniformLocation(blendProgram, "rate");

    const SDL_Color tint = Config::COLLISION_OVERLAY_COLOR;
    glUseProgram(outlineProgram);
    glUniform1f(glGetUniformLocation(outlineProgram, "flip"), -1.0f);
    glUniform1i(glGetUniformLocation(outlineProgram, "field"), 0);
    glUniform3f(glGetUniformLocation(outlineProgram, "tint"), tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f);
    glUniform1f(glGetUniformLocation(outlineProgram, "edgeAlpha"), Config::COLLISION_OVERLAY_EDGE_ALPHA / 255.0f);
    thresholdLocation = glGetUniformLocation(outlineProgram, "threshold");
    glUseProgram(0);

    glGenFramebuffers(1, &fbo);
    glGenVertexArrays(1, &vao);
    for (Readback& slot : readbacks) {
        glGenBuffers(1, &slot.buffer);
    }
}

CollisionField::~CollisionField() {
    for (Readback& slot : readbacks) {
        if (slot.sync) glDeleteSync(slot.sync);
        glDeleteBuffers(1, &slot.buffer);
    }
    glDeleteTextures(2, textures);
    glDeleteTextures(2, levels);
    glDeleteTextures(2, history);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteProgram(outlineProgram);
    glDeleteProgram(blendProgram);
    glDeleteProgram(blurProgram);
    glDeleteProgram(solidityProgram);
}

void CollisionField::resize(int width, int height) {
    if (width == fieldWidth && height == fieldHeight) return;

    glDeleteTextures(2, textures);
    glDeleteTextures(2, levels);
    glDeleteTextures(2, history);
    textures[0] = createFieldTexture(width, height, GL_R8, false);
    textures[1] = createFieldTexture(width, height, GL_R8, false);
    levels[0] = createFieldTexture(width, height, GL_R32F, true);
    levels[1] = createFieldTexture(width, height, GL_R32F, true);
    history[0] = createFieldTexture(width, height, GL_R16, false);
    history[1] = createFieldTexture(width, height, GL_R16, false);
    historyValid = false;
    fieldWidth = width;
    fieldHeight = height;
}

void CollisionField::steady(float rate) {
    glUseProgram(blendProgram);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, history[1], 0);
    glUniform1f(rateLocation, rate);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, history[0]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);
    glUniform1f(rateLocation, 1.0f);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, history[1]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, history[1]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    std::swap(history[0], history[1]);
}

void CollisionField::blur(GLuint* pair, float radius, int lod) {
    if (radius < 1.0f) return;

    const int width = std::max(1, fieldWidth >> lod);
    const int height = std::max(1, fieldHeight >> lod);

    glViewport(0, 0, width, height);
    glUseProgram(blurProgram);
    glUniform1f(radiusLocation, radius);
    glUniform1f(lodLocation, static_cast<float>(lod));

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pair[1], lod);
    glUniform2f(axisLocation, 1.0f / width, 0.0f);
    glBindTexture(GL_TEXTURE_2D, pair[0]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pair[0], lod);
    glUniform2f(axisLocation, 0.0f, 1.0f / height);
    glBindTexture(GL_TEXTURE_2D, pair[1]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glViewport(0, 0, fieldWidth, fieldHeight);
}

void CollisionField::collect() {
    Readback* newest = nullptr;
    for (Readback& slot : readbacks) {
        if (!slot.sync) continue;

        const GLenum state = glClientWaitSync(slot.sync, 0, 0);
        if (state == GL_TIMEOUT_EXPIRED) continue;

        glDeleteSync(slot.sync);
        slot.sync = nullptr;
        if (state == GL_WAIT_FAILED) continue;
        if (!newest || slot.order > newest->order) newest = &slot;
    }
    if (!newest || newest->order < received) return;

    const size_t size = static_cast<size_t>(newest->width) * newest->height;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, newest->buffer);
    if (const void* mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, size, GL_MAP_READ_BIT)) {
        const unsigned char* bytes = static_cast<const unsigned char*>(mapped);
        data.assign(bytes, bytes + size);
        dataWidth = newest->width;
        dataHeight = newest->height;
        received = newest->order;
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void CollisionField::request() {
    for (Readback& slot : readbacks) {
        if (slot.sync) continue;

        glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.buffer);
        if (slot.width != fieldWidth || slot.height != fieldHeight) {
            glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(fieldWidth) * fieldHeight, nullptr, GL_STREAM_READ);
            slot.width = fieldWidth;
            slot.height = fieldHeight;
        }
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, fieldWidth, fieldHeight, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        slot.sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        slot.order = ++issued;
        return;
    }
}

void CollisionField::update(GLuint canvas, int newWindowWidth, int newWindowHeight, float deltaTime) {
    windowWidth = newWindowWidth;
    windowHeight = newWindowHeight;
    resize(std::max(1, windowWidth / Config::COLLISION_DOWNSCALE), std::max(1, windowHeight / Config::COLLISION_DOWNSCALE));
    collect();

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fieldWidth, fieldHeight);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);

    const float scale = static_cast<float>(fieldWidth) / std::max(windowWidth, 1);
    const int lod = Config::COLLISION_SURROUND_LOD;

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, levels[0], 0);
    glUseProgram(solidityProgram);
    glUniform1i(measureLocation, 1);
    glBindTexture(GL_TEXTURE_2D, canvas);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, levels[0]);
    glGenerateMipmap(GL_TEXTURE_2D);
    blur(levels, neighborhood * scale / (1 << lod), lod);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);
    glUseProgram(solidityProgram);
    glUniform1i(measureLocation, 0);
    glUniform1f(floorLocation, floorLevel);
    glUniform1f(fractionLocation, fraction);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, levels[0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, canvas);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    blur(textures, smoothness * scale, 0);

    if (timeSmoothing > 0.0f) {
        steady(historyValid ? 1.0f - std::exp(-deltaTime * 1000.0f / timeSmoothing) : 1.0f);
        historyValid = true;
    } else {
        historyValid = false;
    }

    request();

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_BLEND);
}

void CollisionField::renderOutline() {
    if (!visible || fieldWidth == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(outlineProgram);
    glUniform1f(thresholdLocation, threshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

float CollisionField::sample(float x, float y) const {
    if (windowWidth <= 0 || windowHeight <= 0) return 0.0f;
    return sampleGrid(data.data(), dataWidth, dataHeight, x / windowWidth, y / windowHeight);
}

float CollisionField::depth(float x, float y) const {
    return sample(x, y) - threshold;
}

bool CollisionField::solid(float x, float y) const {
    return depth(x, y) > 0.0f;
}