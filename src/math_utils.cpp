#include "math_utils.h"
#include <cmath>
#include <algorithm>

namespace MathUtils {
    void hsvToRgb(float h, float s, float v, Uint8& r, Uint8& g, Uint8& b) {
        if (s <= 0.0f) {
            r = g = b = static_cast<Uint8>(v * 255);
            return;
        }

        h = fmod(h, 1.0f) * 6.0f;
        int i = static_cast<int>(h);
        float f = h - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i) {
        case 0: r = static_cast<Uint8>(v * 255); g = static_cast<Uint8>(t * 255); b = static_cast<Uint8>(p * 255); break;
        case 1: r = static_cast<Uint8>(q * 255); g = static_cast<Uint8>(v * 255); b = static_cast<Uint8>(p * 255); break;
        case 2: r = static_cast<Uint8>(p * 255); g = static_cast<Uint8>(v * 255); b = static_cast<Uint8>(t * 255); break;
        case 3: r = static_cast<Uint8>(p * 255); g = static_cast<Uint8>(q * 255); b = static_cast<Uint8>(v * 255); break;
        case 4: r = static_cast<Uint8>(t * 255); g = static_cast<Uint8>(p * 255); b = static_cast<Uint8>(v * 255); break;
        default: r = static_cast<Uint8>(v * 255); g = static_cast<Uint8>(p * 255); b = static_cast<Uint8>(q * 255); break;
        }
    }

    void rgbToHsv(Uint8 r, Uint8 g, Uint8 b, float& h, float& s, float& v) {
        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;
        float max = std::max({ rf, gf, bf });
        float min = std::min({ rf, gf, bf });
        float delta = max - min;
        v = max;
        s = (max != 0.0f) ? (delta / max) : 0.0f;

        if (delta == 0.0f) {
            h = 0.0f;
        }
        else if (max == rf) {
            h = fmod(((gf - bf) / delta), 6.0f) / 6.0f;
        }
        else if (max == gf) {
            h = ((bf - rf) / delta + 2.0f) / 6.0f;
        }
        else {
            h = ((rf - gf) / delta + 4.0f) / 6.0f;
        }
        if (h < 0.0f) h += 1.0f;
    }
}