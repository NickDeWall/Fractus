#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <map>
#include <deque>
#include "scene/screen.h"
#include "core/config.h"

class FractalManager {
public:
    FractalManager(int width, int height, const glm::mat4& projection);
    ~FractalManager();

    GLuint processFrame(const std::vector<Screen>& screens, int frameCounter, double time);
    void renderCurrentFrame();
    void renderOutline(const Screen& screen, bool scaling, const glm::mat4& windowProjection);
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
    struct OutlineCache {
        std::vector<float> radii;
        std::vector<float> vertices;
        std::vector<int> strips;
        int shape = -2;
        float params[DisplayModes::MAX_PARAMS] = {};
        float width = 0.0f;
        float height = 0.0f;
    };

    void uploadStack(const Screen& screen);
    const OutlineCache& outlineGeometry(const Screen& screen);
    void copyFrame(GLuint source, int sourceW, int sourceH, GLuint destination, int destinationW, int destinationH);
    void deleteDelayState(DelayState& state);
    void clearSnapshots();
    GLuint screenTexture(const Screen& screen) const;

    int width, height;
    int renderWidth, renderHeight;
    static void computeRenderSize(int w, int h, int& outW, int& outH);
    GLuint compositeShaderProgram;
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
    GLuint outlineVao, outlineVbo;
    std::unordered_map<int, OutlineCache> outlines;
};