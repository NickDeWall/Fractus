#include "fractal_manager.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <string>

FractalManager::FractalManager(int width, int height, GLuint textureShader, GLuint colorShader, const glm::mat4& projection)
    : width(width), height(height), textureShaderProgram(textureShader), colorShaderProgram(colorShader) {
    currentTexture = createTexture(width, height);
    previousTexture = createTexture(width, height);
    
    this->projection = projection;

    glGenFramebuffers(1, &fbo);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, previousTexture, 0);
    glViewport(0, 0, width, height);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

GLuint FractalManager::processFrame(const std::vector<Screen>& screens, int frameCounter) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currentTexture, 0);
    
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glm::mat4 offscreenProjection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    
    for (const auto& screen : screens) {
        glUseProgram(textureShaderProgram);
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(screen.getX(), height - screen.getY(), 0.0f));
        model = glm::rotate(model, glm::radians(-screen.getRotation() + 180), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(screen.getWidth()/2.0f, -screen.getHeight()/2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(-(float)screen.getWidth(), (float)screen.getHeight(), 1.0f));
        
        GLint projLoc = glGetUniformLocation(textureShaderProgram, "projection");
        GLint modelLoc = glGetUniformLocation(textureShaderProgram, "model");
        GLint texLoc = glGetUniformLocation(textureShaderProgram, "tex");
        GLint colorLoc = glGetUniformLocation(textureShaderProgram, "color");
        
        if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &offscreenProjection[0][0]);
        if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        if (colorLoc != -1) glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, previousTexture);
        if (texLoc != -1) glUniform1i(texLoc, 0);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        glUseProgram(0);
        
        SDL_Color color = screen.getColor();
        float alpha = color.a / 255.0f;
        
        glUseProgram(colorShaderProgram);
        
        projLoc = glGetUniformLocation(colorShaderProgram, "projection");
        modelLoc = glGetUniformLocation(colorShaderProgram, "model");
        colorLoc = glGetUniformLocation(colorShaderProgram, "color");
        
        if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &offscreenProjection[0][0]);
        if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        if (colorLoc != -1) glUniform4f(colorLoc, color.r / 255.0f * alpha, color.g / 255.0f * alpha, color.b / 255.0f * alpha, alpha);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    std::swap(currentTexture, previousTexture);
    
    return previousTexture;
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