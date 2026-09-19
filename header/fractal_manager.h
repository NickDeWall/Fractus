#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <map>
#include <deque>
#include "screen.h"
#include "config.h"
#include <SDL2/SDL_ttf.h>
#include <string>

class FractalManager {
public:
    FractalManager(int width, int height, GLuint textureShader, GLuint colorShader, const glm::mat4& projection);
    ~FractalManager();

    GLuint processFrame(const std::vector<Screen>& screens, int frameCounter, double time);
    void renderCurrentFrame();
    GLuint loadPreviousFrame(int frameNum);
    void saveFrame(GLuint texture, int frameNum);
    void resize(int newWidth, int newHeight, const glm::mat4& newProjection);
    

private:
    GLuint createTexture(int w, int h);

    struct Snapshot {
        GLuint texture;
        long long tick;
        bool used;
    };

    struct Frame {
        GLuint texture = 0;
        int width = 0;
        int height = 0;
        double time = 0.0;
    };

    struct DelayState {
        std::deque<Frame> history;
        Frame held;
        long long captureTick = -1;
        long long refreshTick = -1;
        bool used = false;
    };

    void updateSnapshots(const std::vector<Screen>& screens, double time);
    void updateDelayed(const Screen& screen, double time);
    void historySize(const Screen& screen, int& outW, int& outH) const;
    void storeFrame(Frame& destination, GLuint source, int sourceW, int sourceH, int w, int h);
    void copyFrame(GLuint source, int sourceW, int sourceH, GLuint destination, int destinationW, int destinationH);
    void deleteDelayState(DelayState& state);
    void clearSnapshots();
    GLuint screenTexture(const Screen& screen) const;

    int width, height;
    int renderWidth, renderHeight;
    static void computeRenderSize(int w, int h, int& outW, int& outH);
    GLuint textureShaderProgram;
    GLuint colorShaderProgram;
    GLuint screenShaderProgram;
    glm::mat4 projection;

    // Simplified texture management - ping-pong between two textures
    GLuint currentTexture;
    GLuint previousTexture;
    
    // OpenGL objects
    GLuint fbo;
    GLuint snapshotFbo;
    std::map<float, Snapshot> snapshots;
    std::vector<GLuint> spareTextures;
    std::unordered_map<int, DelayState> delayed;
    GLuint vao, vbo;
};

namespace OtherRenders {
    void renderSelectionOutline(Screen* selectedScreen, bool scalingMode, int tempWidth, int tempHeight, GLuint colorShaderProgram, const glm::mat4& projection, GLuint vao);
    void initGL(int width, int height, GLuint& vao, GLuint& vbo);
    void cleanupGL(GLuint& vao, GLuint& vbo);
    bool initFPSTexture(GLuint& fpsTexture);
    bool updateFPSTexture(int fps, TTF_Font* font, GLuint& fpsTexture, int& outWidth, int& outHeight);
    void renderFPS(int screenWidth, int screenHeight, GLuint fpsTexture, int fpsWidth, int fpsHeight, GLuint textureShaderProgram, const glm::mat4& projection, GLuint vao);
    bool initDebugTexture(GLuint& debugTexture);
    bool updateDebugTexture(const std::string& debugText, TTF_Font* font, GLuint& debugTexture, int& outWidth, int& outHeight);
    void renderDebugText(int screenWidth, int screenHeight, GLuint debugTexture, int debugWidth, int debugHeight, GLuint textureShaderProgram, const glm::mat4& projection, GLuint vao);
}