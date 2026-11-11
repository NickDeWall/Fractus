#pragma once
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Window {
public:
    Window(const char* title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    SDL_Window* handle() const { return window; }
    SDL_GLContext context() const { return glContext; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const glm::mat4& getProjection() const { return projection; }

    bool isMinimized() const { return minimized; }
    bool isFullscreen() const { return borderlessFullscreen; }
    void setFullscreen(bool enable);
    void toggleFullscreen();

    bool handleEvent(const SDL_Event& event);
    bool applyPendingResize();
    void swap() const;
    void logInfo() const;

private:
    SDL_Window* window;
    SDL_GLContext glContext;
    glm::mat4 projection;
    SDL_Rect windowedRect;
    int width;
    int height;
    bool minimized;
    bool pendingResize;
    bool borderlessFullscreen;

    void updateProjection();
};