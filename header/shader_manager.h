#pragma once
#include <GL/glew.h>
#include <stdexcept>
#include <string>

namespace ShaderManager {
    GLuint createShaderProgram(const char* vertexSrc, const char* fragmentSrc);
}