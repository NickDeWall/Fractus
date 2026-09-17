#include "input_manager.h"
#include <iostream>

#ifdef _WIN32
#undef main
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