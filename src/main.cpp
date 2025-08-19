#include "input_manager.h"

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
        return 1;
    }
}