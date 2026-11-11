#include "render/overlay_renderer.h"
#include "render/shader_manager.h"
#include "render/font_data.h"
#include "core/config.h"
#include "util/math_utils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

namespace {
    const char* VERTEX_SHADER = R"(
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

    const char* FRAGMENT_SHADER = R"(
        #version 330 core
        in vec2 vTexCoord;
        uniform sampler2D tex;
        uniform vec4 color;
        out vec4 fragColor;
        void main() {
            fragColor = texture(tex, vTexCoord) * color;
        }
    )";
}

OverlayRenderer::OverlayRenderer()
    : font(nullptr), shaderProgram(0), vao(0), vbo(0),
      fpsTexture(0), fpsWidth(0), fpsHeight(0), lastFpsTime(0),
      debugTexture(0), debugWidth(0), debugHeight(0) {
    if (TTF_Init() == -1) {
        throw std::runtime_error("Failed to initialize SDL_ttf");
    }

    font = TTF_OpenFontRW(SDL_RWFromConstMem(BebasNeue_Regular_ttf,
        static_cast<int>(BebasNeue_Regular_ttf_len)), 1, 24);

    shaderProgram = ShaderManager::createShaderProgram(VERTEX_SHADER, FRAGMENT_SHADER);

    const float vertices[] = {
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

    fpsTexture = createLabelTexture();
    debugTexture = createLabelTexture();
    lastFpsTime = SDL_GetTicks();
}

OverlayRenderer::~OverlayRenderer() {
    glDeleteTextures(1, &fpsTexture);
    glDeleteTextures(1, &debugTexture);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);

    if (font) TTF_CloseFont(font);
    TTF_Quit();
}

GLuint OverlayRenderer::createLabelTexture() {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const unsigned char emptyPixels[4] = { 0, 0, 0, 0 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, emptyPixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

bool OverlayRenderer::renderLabel(const std::string& text, GLuint texture, int& outWidth, int& outHeight) const {
    if (!font || text.empty()) {
        outWidth = 0;
        outHeight = 0;
        return false;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), Config::FPS_TEXT_COLOR);
    if (!surface) return false;

    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return false;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    outWidth = rgba->w;
    outHeight = rgba->h;
    SDL_FreeSurface(rgba);
    return true;
}

void OverlayRenderer::drawLabel(GLuint texture, float x, float y, int labelWidth, int labelHeight,
    const glm::mat4& projection) const {
    if (labelWidth == 0 || labelHeight == 0) return;

    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint srcBlend = 0;
    GLint dstBlend = 0;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcBlend);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstBlend);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(labelWidth, labelHeight, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform4f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    if (!blendEnabled) glDisable(GL_BLEND);
    else glBlendFunc(srcBlend, dstBlend);
}

void OverlayRenderer::drawFps(int windowWidth, int windowHeight, const glm::mat4& projection) {
    const Uint32 now = SDL_GetTicks();
    const float fps = MathUtils::calculateFPS();

    if (now - lastFpsTime >= static_cast<Uint32>(Config::FPS_UPDATE_INTERVAL_MS)) {
        renderLabel(std::to_string(static_cast<int>(fps)), fpsTexture, fpsWidth, fpsHeight);
        lastFpsTime = now;
    }

    drawLabel(fpsTexture, static_cast<float>(windowWidth - fpsWidth), 0.0f, fpsWidth, fpsHeight, projection);
}

void OverlayRenderer::setDebugText(const std::string& text) {
    renderLabel(text, debugTexture, debugWidth, debugHeight);
}

void OverlayRenderer::drawDebugText(int windowWidth, int windowHeight, const glm::mat4& projection) {
    drawLabel(debugTexture, static_cast<float>(windowWidth / 2),
        static_cast<float>(windowHeight - debugHeight), debugWidth, debugHeight, projection);
}