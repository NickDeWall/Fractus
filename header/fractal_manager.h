
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include "screen.h"
#include "config.h"

class FractalManager {
public:
    FractalManager(int width, int height, GLuint textureShader, GLuint colorShader, const glm::mat4& projection);
    ~FractalManager();

    GLuint processFrame(const std::vector<Screen>& screens, int frameCounter);
    void renderCurrentFrame();
    GLuint loadPreviousFrame(int frameNum);
    void saveFrame(GLuint texture, int frameNum);
    

private:
    GLuint createTexture(int w, int h);

    int width, height;
    GLuint textureShaderProgram;
    GLuint colorShaderProgram;
    glm::mat4 projection;

    // Simplified texture management - ping-pong between two textures
    GLuint currentTexture;
    GLuint previousTexture;
    
    // OpenGL objects
    GLuint fbo;
    GLuint vao, vbo;
};

namespace OtherRenders {
    void drawSelectionOutline(Screen* selectedScreen, bool scalingMode, int tempWidth, int tempHeight, GLuint colorShaderProgram, const glm::mat4& projection, GLuint vao);
    void initGL(int width, int height, GLuint& vao, GLuint& vbo);
    void cleanupGL(GLuint& vao, GLuint& vbo);
}