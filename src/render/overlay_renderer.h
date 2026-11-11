#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <SDL2/SDL_ttf.h>
#include <string>

class OverlayRenderer {
public:
    OverlayRenderer();
    ~OverlayRenderer();

    OverlayRenderer(const OverlayRenderer&) = delete;
    OverlayRenderer& operator=(const OverlayRenderer&) = delete;

    void drawFps(int windowWidth, int windowHeight, const glm::mat4& projection);
    void setDebugText(const std::string& text);
    void drawDebugText(int windowWidth, int windowHeight, const glm::mat4& projection);

private:
    TTF_Font* font;
    GLuint shaderProgram;
    GLuint vao;
    GLuint vbo;

    GLuint fpsTexture;
    int fpsWidth;
    int fpsHeight;
    Uint32 lastFpsTime;

    GLuint debugTexture;
    int debugWidth;
    int debugHeight;

    static GLuint createLabelTexture();
    bool renderLabel(const std::string& text, GLuint texture, int& outWidth, int& outHeight) const;
    void drawLabel(GLuint texture, float x, float y, int labelWidth, int labelHeight, const glm::mat4& projection) const;
};