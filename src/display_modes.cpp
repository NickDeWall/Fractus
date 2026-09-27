#include "display_modes.h"
#include "config.h"
#include <algorithm>
#include <cmath>

namespace {
    const char* SHADER_PRELUDE = R"(
        #version 330 core
        const float TAU = 6.28318530718;
        const float PI = 3.14159265359;
        in vec2 vTexCoord;
        uniform sampler2D tex;
        uniform vec4 color;
        uniform int mode;
        uniform float aspect;
        uniform float params[PARAM_COUNT];
        out vec4 fragColor;

        vec3 rgbToHsv(vec3 rgb) {
            float maxComponent = max(rgb.r, max(rgb.g, rgb.b));
            float minComponent = min(rgb.r, min(rgb.g, rgb.b));
            float span = maxComponent - minComponent;
            float hue = 0.0;
            if (span > 0.0) {
                if (maxComponent == rgb.r) hue = mod((rgb.g - rgb.b) / span, 6.0);
                else if (maxComponent == rgb.g) hue = (rgb.b - rgb.r) / span + 2.0;
                else hue = (rgb.r - rgb.g) / span + 4.0;
                hue /= 6.0;
            }
            float saturation = maxComponent > 0.0 ? span / maxComponent : 0.0;
            return vec3(hue, saturation, maxComponent);
        }

        vec3 hsvToRgb(vec3 hsv) {
            vec3 k = mod(vec3(5.0, 3.0, 1.0) + hsv.x * 6.0, 6.0);
            return hsv.z * (1.0 - hsv.y * clamp(min(k, 4.0 - k), 0.0, 1.0));
        }

        vec2 plane() {
            return (vTexCoord - 0.5) * vec2(aspect, 1.0);
        }

        vec2 toTexture(vec2 point) {
            return vec2(0.5 + point.x / aspect, 0.5 + point.y);
        }

        bool offCanvas(vec2 uv) {
            return any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)));
        }

        void main() {
            vec2 uv = vTexCoord;
)";

    const std::vector<ModeInfo>& table() {
        static const std::vector<ModeInfo> modes = {
            { "Normal", ModeKind::None, "", {} },
            { "Prop", ModeKind::Prop, "", {} },
            { "Log-Polar", ModeKind::Geometry, R"(
                float maxRadius = 0.5 * min(aspect, 1.0);
                float lowRadius = maxRadius * P0;
                float radius = lowRadius * exp(vTexCoord.x * log(maxRadius / lowRadius));
                float angle = vTexCoord.y * TAU;
                uv = toTexture(radius * vec2(cos(angle), sin(angle)));
            )", {
                { "Min radius %.4f", ParamKind::Float, Config::LOG_POLAR_MIN_RADIUS_LOW, Config::LOG_POLAR_MIN_RADIUS_HIGH, Config::LOG_POLAR_MIN_RADIUS, true },
            } },
            { "Julia (z\xC2\xB2 + c)", ModeKind::Geometry, R"(
                vec2 span = vec2(aspect, 1.0) * JULIA_HEIGHT;
                vec2 z = (vTexCoord - 0.5) * span;
                z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + vec2(P0, P1);
                uv = z / span + 0.5;
                if (offCanvas(uv)) { fragColor = vec4(0.0); return; }
            )", {
                { "c real %.4f", ParamKind::Float, -Config::JULIA_C_LIMIT, Config::JULIA_C_LIMIT, Config::JULIA_DEFAULT_REAL, false },
                { "c imag %.4f", ParamKind::Float, -Config::JULIA_C_LIMIT, Config::JULIA_C_LIMIT, Config::JULIA_DEFAULT_IMAG, false },
            } },
            { "Droste", ModeKind::Geometry, R"(
                vec2 z = plane();
                float turn = fract(atan(z.y, z.x) / TAU);
                float rings = log(max(length(z), 1e-6)) / log(P0);
                uv = vec2(fract(rings + P1 * turn), turn);
            )", {
                { "Zoom per ring %.2fx", ParamKind::Float, Config::DROSTE_MIN_ZOOM, Config::DROSTE_MAX_ZOOM, Config::DROSTE_DEFAULT_ZOOM, true },
                { "Spiral arms %d", ParamKind::Integer, -Config::DROSTE_ARM_LIMIT, Config::DROSTE_ARM_LIMIT, Config::DROSTE_DEFAULT_ARMS, false },
            } },
            { "Power (z\xE2\x81\xBF)", ModeKind::Geometry, R"(
                vec2 z = plane();
                float radius = pow(max(length(z), 1e-6) * 2.0, P0) * 0.5;
                float angle = atan(z.y, z.x) * P0;
                uv = toTexture(radius * vec2(cos(angle), sin(angle)));
                if (offCanvas(uv)) { fragColor = vec4(0.0); return; }
            )", {
                { "Power %.2f", ParamKind::Float, -Config::POWER_LIMIT, Config::POWER_LIMIT, Config::POWER_DEFAULT, false },
            } },
            { "Kaleidoscope", ModeKind::Geometry, R"(
                vec2 z = plane();
                float offset = P1 * PI / 180.0;
                float wedge = TAU / P0;
                float angle = mod(atan(z.y, z.x) - offset, wedge);
                angle = min(angle, wedge - angle) + offset;
                uv = toTexture(length(z) * vec2(cos(angle), sin(angle)));
                if (offCanvas(uv)) { fragColor = vec4(0.0); return; }
            )", {
                { "Segments %d", ParamKind::Integer, Config::KALEIDOSCOPE_MIN_SEGMENTS, Config::KALEIDOSCOPE_MAX_SEGMENTS, Config::KALEIDOSCOPE_DEFAULT_SEGMENTS, false },
                { "Wedge angle %.1f\xC2\xB0", ParamKind::Float, 0.0f, 360.0f, 0.0f, false },
            } },
            { "Inversion (1/z)", ModeKind::Geometry, R"(
                vec2 z = plane();
                float lengthSquared = max(dot(z, z), 1e-8);
                uv = toTexture(z * (P0 * P0 * 0.25) / lengthSquared);
                if (offCanvas(uv)) { fragColor = vec4(0.0); return; }
            )", {
                { "Circle radius %.3f", ParamKind::Float, Config::INVERSION_MIN_RADIUS, Config::INVERSION_MAX_RADIUS, Config::INVERSION_DEFAULT_RADIUS, true },
            } },
            { "Swirl", ModeKind::Geometry, R"(
                vec2 z = plane();
                float extent = max(P1 * 0.5, 1e-6);
                float falloff = max(1.0 - length(z) / extent, 0.0);
                float angle = P0 * TAU * falloff * falloff;
                float sine = sin(angle);
                float cosine = cos(angle);
                uv = toTexture(vec2(z.x * cosine - z.y * sine, z.x * sine + z.y * cosine));
                if (offCanvas(uv)) { fragColor = vec4(0.0); return; }
            )", {
                { "Swirl %.2f turns", ParamKind::Float, -Config::SWIRL_STRENGTH_LIMIT, Config::SWIRL_STRENGTH_LIMIT, Config::SWIRL_DEFAULT_STRENGTH, false },
                { "Swirl radius %.2f", ParamKind::Float, Config::SWIRL_MIN_RADIUS, Config::SWIRL_MAX_RADIUS, Config::SWIRL_DEFAULT_RADIUS, false },
            } },
            { "Tile / Mirror", ModeKind::Geometry, R"(
                vec2 scaled = vTexCoord * P0;
                vec2 cell = fract(scaled);
                vec2 flipped = mod(floor(scaled), 2.0) * step(0.5, P1);
                uv = mix(cell, 1.0 - cell, flipped);
            )", {
                { "Tiles %d", ParamKind::Integer, Config::TILE_MIN_COUNT, Config::TILE_MAX_COUNT, Config::TILE_DEFAULT_COUNT, false },
                { "Mirror tiles", ParamKind::Boolean, 0.0f, 1.0f, 1.0f, false },
            } },
            { "Sharpen", ModeKind::Sampling, R"(
                vec2 texel = 1.0 / vec2(textureSize(tex, 0));
                vec4 middle = texture(tex, uv);
                vec4 neighbours = texture(tex, uv + vec2(texel.x, 0.0)) + texture(tex, uv - vec2(texel.x, 0.0))
                    + texture(tex, uv + vec2(0.0, texel.y)) + texture(tex, uv - vec2(0.0, texel.y));
                fragColor = max(middle + P0 * (middle * 4.0 - neighbours) * 0.25, vec4(0.0)) * color;
                return;
            )", {
                { "Sharpen %.2f", ParamKind::Float, 0.0f, Config::SHARPEN_STRENGTH_LIMIT, Config::SHARPEN_DEFAULT_STRENGTH, false },
            } },
            { "Hue shift", ModeKind::Color, R"(
                vec4 source = texture(tex, uv);
                vec3 hsv = rgbToHsv(source.rgb);
                hsv.x = fract(hsv.x + P0);
                fragColor = vec4(hsvToRgb(hsv), source.a) * color;
                return;
            )", {
                { "Hue %.3f turns", ParamKind::Float, -Config::HUE_SHIFT_LIMIT, Config::HUE_SHIFT_LIMIT, Config::HUE_SHIFT_DEFAULT, false },
            } },
            { "M\xC3\xB6" "bius", ModeKind::Geometry, R"(
                vec2 z = plane();
                vec2 numerator = z + vec2(P0, P1);
                vec2 denominator = vec2(P2 * z.x - P3 * z.y, P2 * z.y + P3 * z.x) + vec2(1.0, 0.0);
                float lengthSquared = max(dot(denominator, denominator), 1e-8);
                vec2 w = vec2(dot(numerator, denominator), numerator.y * denominator.x - numerator.x * denominator.y) / lengthSquared;
                uv = toTexture(w);
                if (offCanvas(uv)) { fragColor = vec4(0.0); return; }
            )", {
                { "shift real %.3f", ParamKind::Float, -Config::MOBIUS_B_LIMIT, Config::MOBIUS_B_LIMIT, Config::MOBIUS_DEFAULT_B_REAL, false },
                { "shift imag %.3f", ParamKind::Float, -Config::MOBIUS_B_LIMIT, Config::MOBIUS_B_LIMIT, Config::MOBIUS_DEFAULT_B_IMAG, false },
                { "bend real %.3f", ParamKind::Float, -Config::MOBIUS_C_LIMIT, Config::MOBIUS_C_LIMIT, Config::MOBIUS_DEFAULT_C_REAL, false },
                { "bend imag %.3f", ParamKind::Float, -Config::MOBIUS_C_LIMIT, Config::MOBIUS_C_LIMIT, Config::MOBIUS_DEFAULT_C_IMAG, false },
            } },
            { "Invert", ModeKind::Color, R"(
                vec4 source = texture(tex, uv);
                vec3 straight = source.a > 0.0 ? source.rgb / source.a : source.rgb;
                vec3 flipped;
                if (P2 > 0.5) {
                    vec3 hsv = rgbToHsv(straight);
                    hsv.x = fract(hsv.x + 0.5 * P0);
                    flipped = hsvToRgb(hsv);
                } else {
                    flipped = mix(straight, vec3(1.0) - straight, P0);
                }
                flipped = clamp((flipped - 0.5) * P1 + 0.5, 0.0, 1.0);
                fragColor = vec4(flipped * source.a, source.a) * color;
                return;
            )", {
                { "Invert %.2f", ParamKind::Float, 0.0f, 1.0f, Config::INVERT_DEFAULT_AMOUNT, false },
                { "Contrast %.2f", ParamKind::Float, Config::INVERT_MIN_CONTRAST, Config::INVERT_MAX_CONTRAST, Config::INVERT_DEFAULT_CONTRAST, false },
                { "Hue flip", ParamKind::Boolean, 0.0f, 1.0f, 1.0f, false },
            } },
            { "Chromatic split", ModeKind::Sampling, R"(
                vec2 offset = vTexCoord - 0.5;
                vec4 source = texture(tex, uv);
                float red = texture(tex, 0.5 + offset * (1.0 - P0)).r;
                float blue = texture(tex, 0.5 + offset * (1.0 + P0)).b;
                fragColor = vec4(red, source.g, blue, source.a) * color;
                return;
            )", {
                { "Split %.4f", ParamKind::Float, -Config::CHROMATIC_SPLIT_LIMIT, Config::CHROMATIC_SPLIT_LIMIT, Config::CHROMATIC_DEFAULT_SPLIT, false },
            } },
            { "Newton (z\xE2\x81\xBF - 1)", ModeKind::Geometry, R"(
                vec2 span = vec2(aspect, 1.0) * NEWTON_HEIGHT;
                vec2 z = (vTexCoord - 0.5) * span;
                float radius = max(length(z), 1e-6);
                float angle = atan(z.y, z.x);
                vec2 zPower = pow(radius, P0) * vec2(cos(P0 * angle), sin(P0 * angle)) - vec2(1.0, 0.0);
                float derivativeAngle = (P0 - 1.0) * angle;
                vec2 derivative = P0 * pow(radius, P0 - 1.0) * vec2(cos(derivativeAngle), sin(derivativeAngle));
                float lengthSquared = max(dot(derivative, derivative), 1e-8);
                vec2 quotient = vec2(dot(zPower, derivative), zPower.y * derivative.x - zPower.x * derivative.y) / lengthSquared;
                uv = (z - P1 * quotient) / span + 0.5;
                if (offCanvas(uv)) { fragColor = vec4(0.0); return; }
            )", {
                { "Roots %d", ParamKind::Integer, Config::NEWTON_MIN_ORDER, Config::NEWTON_MAX_ORDER, Config::NEWTON_DEFAULT_ORDER, false },
                { "Step %.2f", ParamKind::Float, Config::NEWTON_MIN_STEP, Config::NEWTON_MAX_STEP, Config::NEWTON_DEFAULT_STEP, false },
            } },
            { "Shear", ModeKind::Geometry, R"(
                vec2 z = plane();
                uv = toTexture(vec2(z.x + P0 * z.y, z.y + P1 * z.x));
                if (offCanvas(uv)) { fragColor = vec4(0.0); return; }
            )", {
                { "Shear X %.3f", ParamKind::Float, -Config::SHEAR_LIMIT, Config::SHEAR_LIMIT, 0.2f, false },
                { "Shear Y %.3f", ParamKind::Float, -Config::SHEAR_LIMIT, Config::SHEAR_LIMIT, 0.0f, false },
            } },
        };
        return modes;
    }

    std::string replaceAll(std::string text, const std::string& from, const std::string& to) {
        for (size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size())) {
            text.replace(at, from.size(), to);
        }
        return text;
    }

    std::string number(float value) {
        return std::to_string(value);
    }
}

namespace DisplayModes {
    const std::vector<ModeInfo>& all() {
        return table();
    }

    const ModeInfo& get(int mode) {
        const std::vector<ModeInfo>& modes = table();
        return modes[std::clamp(mode, 0, static_cast<int>(modes.size()) - 1)];
    }

    int count() {
        return static_cast<int>(table().size());
    }

    int paramCount(int mode) {
        return static_cast<int>(get(mode).params.size());
    }

    bool isProp(int mode) {
        return get(mode).kind == ModeKind::Prop;
    }

    float clampParam(int mode, int param, float value) {
        const ModeInfo& info = get(mode);
        if (param < 0 || param >= static_cast<int>(info.params.size())) return value;

        const ModeParam& slot = info.params[param];
        value = std::clamp(value, slot.minimum, slot.maximum);
        if (slot.kind == ParamKind::Integer) value = std::round(value);
        if (slot.kind == ParamKind::Boolean) value = value > 0.5f ? 1.0f : 0.0f;
        return value;
    }

    std::string buildFragmentShader() {
        const std::vector<ModeInfo>& modes = table();
        std::string source = replaceAll(SHADER_PRELUDE, "PARAM_COUNT", std::to_string(count() * MAX_PARAMS));

        for (int mode = 0; mode < count(); ++mode) {
            const ModeInfo& info = modes[mode];
            if (info.glsl[0] == '\0') continue;

            std::string body = info.glsl;
            for (int param = 0; param < MAX_PARAMS; ++param) {
                body = replaceAll(body, "P" + std::to_string(param),
                    "params[" + std::to_string(mode * MAX_PARAMS + param) + "]");
            }
            body = replaceAll(body, "JULIA_HEIGHT", number(Config::JULIA_VIEW_HEIGHT));
            body = replaceAll(body, "NEWTON_HEIGHT", number(Config::NEWTON_VIEW_HEIGHT));

            source += "            if (mode == " + std::to_string(mode) + ") {\n";
            source += body;
            source += "\n            }\n";
        }

        source += "            fragColor = texture(tex, uv) * color;\n        }\n";
        return source;
    }
}