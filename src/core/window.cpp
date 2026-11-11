#include "core/window.h"
#include "core/config.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <fstream>
#include <string>

Window::Window(const char* title, int requestedWidth, int requestedHeight)
    : window(nullptr), glContext(nullptr), windowedRect{ 0, 0, 0, 0 },
      width(requestedWidth), height(requestedHeight),
      minimized(false), pendingResize(false), borderlessFullscreen(false) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        requestedWidth, requestedHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }

    SDL_GL_GetDrawableSize(window, &width, &height);

    if (Config::VSYNC) {
        if (SDL_GL_SetSwapInterval(-1) != 0) {
            SDL_GL_SetSwapInterval(1);
        }
    } else {
        SDL_GL_SetSwapInterval(0);
    }

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize GLEW");
    }
    glGetError();

    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    updateProjection();
}

Window::~Window() {
    if (glContext) SDL_GL_DeleteContext(glContext);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window::updateProjection() {
    projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
}

bool Window::handleEvent(const SDL_Event& event) {
    if (event.type != SDL_WINDOWEVENT) return false;

    switch (event.window.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED:
        pendingResize = true;
        break;
    case SDL_WINDOWEVENT_MINIMIZED:
        minimized = true;
        break;
    case SDL_WINDOWEVENT_RESTORED:
    case SDL_WINDOWEVENT_SHOWN:
        minimized = false;
        break;
    }

    return true;
}

bool Window::applyPendingResize() {
    if (!pendingResize) return false;
    pendingResize = false;

    int newWidth = 0;
    int newHeight = 0;
    SDL_GL_GetDrawableSize(window, &newWidth, &newHeight);

    if (newWidth <= 0 || newHeight <= 0) return false;
    if (newWidth == width && newHeight == height) return false;

    width = newWidth;
    height = newHeight;
    glViewport(0, 0, width, height);
    updateProjection();
    return true;
}

void Window::setFullscreen(bool enable) {
    if (enable == borderlessFullscreen) return;

    if (enable) {
        SDL_GetWindowPosition(window, &windowedRect.x, &windowedRect.y);
        SDL_GetWindowSize(window, &windowedRect.w, &windowedRect.h);

        SDL_Rect bounds;
        const int displayIndex = SDL_GetWindowDisplayIndex(window);
        if (SDL_GetDisplayBounds(displayIndex < 0 ? 0 : displayIndex, &bounds) != 0) return;

        const int inset = Config::FULLSCREEN_INSET;
        SDL_SetWindowBordered(window, SDL_FALSE);
        SDL_SetWindowPosition(window, bounds.x + inset, bounds.y + inset);
        SDL_SetWindowSize(window, bounds.w - 2 * inset, bounds.h - 2 * inset);
        borderlessFullscreen = true;
    } else {
        SDL_SetWindowBordered(window, SDL_TRUE);
        if (windowedRect.w > 0 && windowedRect.h > 0) {
            SDL_SetWindowSize(window, windowedRect.w, windowedRect.h);
            SDL_SetWindowPosition(window, windowedRect.x, windowedRect.y);
        }
        borderlessFullscreen = false;
    }
}

void Window::toggleFullscreen() {
    setFullscreen(!borderlessFullscreen);
}

void Window::swap() const {
    SDL_GL_SwapWindow(window);
}

void Window::logInfo() const {
    std::ofstream log(Config::LOG_FILE, std::ios::trunc);
    if (!log) return;

    const auto glString = [](GLenum name) -> std::string {
        const GLubyte* value = glGetString(name);
        return value ? reinterpret_cast<const char*>(value) : "(unavailable)";
    };

    log << "VENDOR:        " << glString(GL_VENDOR) << "\n"
        << "RENDERER:      " << glString(GL_RENDERER) << "\n"
        << "GL_VERSION:    " << glString(GL_VERSION) << "\n"
        << "GLSL_VERSION:  " << glString(GL_SHADING_LANGUAGE_VERSION) << "\n"
        << "swap interval: " << SDL_GL_GetSwapInterval() << "\n"
        << "drawable:      " << width << "x" << height << "\n";
}