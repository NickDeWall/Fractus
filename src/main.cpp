#include "input_manager.h"
#include <SDL2/SDL.h>
#include <exception>

#ifdef _WIN32
#undef main

extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main(int argc, char* argv[]) {
    try {
        InputManager visualizer;
        visualizer.run();
        return 0;
    }
    catch (const std::exception& e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fractus failed to start", e.what(), nullptr);
        return 1;
    }
}