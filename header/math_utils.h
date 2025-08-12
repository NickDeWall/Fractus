#pragma once
#include <SDL2/SDL.h>

namespace MathUtils {
    void hsvToRgb(float h, float s, float v, Uint8& r, Uint8& g, Uint8& b);
    void rgbToHsv(Uint8 r, Uint8 g, Uint8 b, float& h, float& s, float& v);
}