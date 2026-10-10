#include "display_modes.h"
#include "config.h"
#include <algorithm>
#include <string>
#include <cmath>

namespace {
    float wrapInto(float value, float period) {
        return value - period * std::floor(value / period);
    }

    float ellipseDistance(const float*, float x, float y) {
        return std::hypot(x, y) - 1.0f;
    }

    float polygonDistance(const float* p, float x, float y) {
        const float turn = 6.28318530718f / p[0];
        const float angle = std::atan2(y, x) + p[1] * 3.14159265359f / 180.0f;
        return std::hypot(x, y) - std::cos(turn * 0.5f) / std::cos(wrapInto(angle + turn * 0.5f, turn) - turn * 0.5f);
    }

    float starDistance(const float* p, float x, float y) {
        const float turn = 6.28318530718f / p[0];
        const float halfTurn = turn * 0.5f;
        const float angle = std::abs(wrapInto(std::atan2(y, x) + p[2] * 3.14159265359f / 180.0f + halfTurn, turn) - halfTurn);
        const float alongX = p[1] * std::cos(halfTurn) - 1.0f;
        const float alongY = p[1] * std::sin(halfTurn);
        const float denominator = std::cos(angle) * alongY - std::sin(angle) * alongX;
        const float edge = std::abs(denominator) < 1e-6f ? 1.0f : alongY / denominator;
        return std::hypot(x, y) - edge;
    }

    float rhombusDistance(const float*, float x, float y) {
        return std::abs(x) + std::abs(y) - 1.0f;
    }

    float roundedDistance(const float* p, float x, float y) {
        const float insideX = std::max(std::abs(x) - (1.0f - p[0]), 0.0f);
        const float insideY = std::max(std::abs(y) - (1.0f - p[0]), 0.0f);
        return std::hypot(insideX, insideY) - p[0];
    }

    float crossDistance(const float* p, float x, float y) {
        return std::min(std::max(std::abs(x) - p[0], std::abs(y) - 1.0f),
            std::max(std::abs(y) - p[0], std::abs(x) - 1.0f));
    }

    float trapezoidDistance(const float* p, float x, float y) {
        const float halfWidth = 1.0f + (p[0] - 1.0f) * (y * 0.5f + 0.5f);
        return std::max(std::abs(y) - 1.0f, std::abs(x) - halfWidth);
    }

    float ringDistance(const float* p, float x, float y) {
        const float radius = std::hypot(x, y);
        return std::max(radius - 1.0f, p[0] - radius);
    }

    const char* SHADER_PRELUDE = R"(
        #version 330 core
        const float TAU = 6.28318530718;
        const float PI = 3.14159265359;
        in vec2 vTexCoord;
        uniform sampler2D tex;
        uniform vec4 color;
        uniform float aspect;
        uniform int overlay;
        uniform vec2 planeScale;
        uniform int stackSize;
        uniform int stackModes[MAX_STACK];
        uniform float stackParams[PARAM_COUNT];
        out vec4 fragColor;

        int gShape = -1;
        float gS0 = 0.0;
        float gS1 = 0.0;
        float gS2 = 0.0;
        float gS3 = 0.0;

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

        vec2 plane(vec2 point) {
            return (point - 0.5) * vec2(aspect, 1.0);
        }

        vec2 toTexture(vec2 point) {
            return vec2(0.5 + point.x / aspect, 0.5 + point.y);
        }

        bool offCanvas(vec2 uv) {
            return any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)));
        }

        vec4 sampleBilinear(vec2 uv) {
            return textureLod(tex, uv, 0.0);
        }

        vec4 sampleBicubic(vec2 uv) {
            vec2 size = vec2(textureSize(tex, 0));
            vec2 texel = 1.0 / size;
            vec2 position = uv * size - 0.5;
            vec2 fraction = fract(position);
            vec2 base = (position - fraction + 0.5) * texel;

            vec2 w0 = fraction * (-0.5 + fraction * (1.0 - 0.5 * fraction));
            vec2 w1 = 1.0 + fraction * fraction * (-2.5 + 1.5 * fraction);
            vec2 w2 = fraction * (0.5 + fraction * (2.0 - 1.5 * fraction));
            vec2 w3 = fraction * fraction * (-0.5 + 0.5 * fraction);

            vec2 middle = w1 + w2;
            vec2 low = base - texel;
            vec2 high = base + texel * 2.0;
            vec2 centre = base + texel * (w2 / middle);

            vec4 result = textureLod(tex, vec2(low.x, low.y), 0.0) * (w0.x * w0.y);
            result += textureLod(tex, vec2(centre.x, low.y), 0.0) * (middle.x * w0.y);
            result += textureLod(tex, vec2(high.x, low.y), 0.0) * (w3.x * w0.y);

            result += textureLod(tex, vec2(low.x, centre.y), 0.0) * (w0.x * middle.y);
            result += textureLod(tex, vec2(centre.x, centre.y), 0.0) * (middle.x * middle.y);
            result += textureLod(tex, vec2(high.x, centre.y), 0.0) * (w3.x * middle.y);

            result += textureLod(tex, vec2(low.x, high.y), 0.0) * (w0.x * w3.y);
            result += textureLod(tex, vec2(centre.x, high.y), 0.0) * (middle.x * w3.y);
            result += textureLod(tex, vec2(high.x, high.y), 0.0) * (w3.x * w3.y);

            return max(result, vec4(0.0));
        }

        vec4 readCanvas(vec2 uv) {
            return offCanvas(uv) ? vec4(0.0) : SAMPLE_FILTER(uv);
        }

        float shapeDistance(vec2 p) {
            float S0 = gS0;
            float S1 = gS1;
            float S2 = gS2;
            float S3 = gS3;
            float d = max(abs(p.x), abs(p.y)) - 1.0;
SHAPE_BRANCHES
            return d;
        }

        float boundaryRadius(vec2 direction) {
            float t = 0.5;
            for (int step = 0; step < 8; ++step) {
                t -= shapeDistance(t * direction);
            }
            return max(t, 1e-4);
        }

        vec2 discToSquare(vec2 disc, float round) {
            float radius = length(disc);
            vec2 direction = radius > 1e-6 ? disc / radius : vec2(1.0, 0.0);
            vec2 radial = direction * radius / max(max(abs(direction.x), abs(direction.y)), 1e-6);

            float sx = disc.x * disc.x;
            float sy = disc.y * disc.y;
            float rootTwo = 1.41421356237;
            vec2 elliptical = vec2(
                0.5 * sqrt(max(2.0 + sx - sy + rootTwo * 2.0 * disc.x, 0.0)) -
                0.5 * sqrt(max(2.0 + sx - sy - rootTwo * 2.0 * disc.x, 0.0)),
                0.5 * sqrt(max(2.0 - sx + sy + rootTwo * 2.0 * disc.y, 0.0)) -
                0.5 * sqrt(max(2.0 - sx + sy - rootTwo * 2.0 * disc.y, 0.0)));

            return mix(radial, elliptical, round);
        }

        void main() {
            vec2 uv = vTexCoord;
            vec4 accum = vec4(0.0);
            vec2 outline = (vTexCoord - 0.5) * 2.0 * planeScale;
            float maskDistance = max(abs(outline.x), abs(outline.y)) - 1.0;
            bool shapeApplied = false;

            for (int entry = 0; entry < stackSize; ++entry) {
                int mode = stackModes[entry];
                float P0 = stackParams[entry * MAX_PARAMS + 0];
                float P1 = stackParams[entry * MAX_PARAMS + 1];
                float P2 = stackParams[entry * MAX_PARAMS + 2];
                float P3 = stackParams[entry * MAX_PARAMS + 3];
                float P4 = stackParams[entry * MAX_PARAMS + 4];
                float P5 = stackParams[entry * MAX_PARAMS + 5];
)";

    std::vector<ModeInfo> withShapeParams(std::vector<ModeInfo> modes) {
        for (ModeInfo& info : modes) {
            if (info.kind != ModeKind::Shape) continue;
            info.params.push_back({ "Fit to shape", ParamKind::Boolean, 0.0f, 1.0f, 1.0f, false });
            if (info.fitGlsl[0] == '\0') {
                info.params.push_back({ "Round fit %.2f", ParamKind::Float, 0.0f, 1.0f, 1.0f, false });
            }
        }
        return modes;
    }

    const std::vector<ModeInfo>& table() {
        static const std::vector<ModeInfo> modes = withShapeParams({
            { "Ellipse", ModeKind::Shape, R"(
                d = length(p) - 1.0;
            )", {}, true, "", ellipseDistance },
            { "Polygon", ModeKind::Shape, R"(
                float turn = TAU / S0;
                float angle = atan(p.y, p.x) + S1 * PI / 180.0;
                d = length(p) - cos(turn * 0.5) / cos(mod(angle + turn * 0.5, turn) - turn * 0.5);
            )", {
                { "Sides %d", ParamKind::Integer, 3.0f, 16.0f, 6.0f, false },
                { "Angle %.0f\xC2\xB0", ParamKind::Float, 0.0f, 360.0f, 90.0f, false },
            }, true, "", polygonDistance },
            { "Star", ModeKind::Shape, R"(
                float turn = TAU / S0;
                float halfTurn = turn * 0.5;
                float angle = abs(mod(atan(p.y, p.x) + S2 * PI / 180.0 + halfTurn, turn) - halfTurn);
                vec2 tip = vec2(1.0, 0.0);
                vec2 along = S1 * vec2(cos(halfTurn), sin(halfTurn)) - tip;
                vec2 ray = vec2(cos(angle), sin(angle));
                float denominator = ray.x * along.y - ray.y * along.x;
                d = length(p) - (abs(denominator) < 1e-6 ? 1.0 : (tip.x * along.y - tip.y * along.x) / denominator);
            )", {
                { "Points %d", ParamKind::Integer, 3.0f, 16.0f, 5.0f, false },
                { "Inner radius %.2f", ParamKind::Float, 0.05f, 0.95f, 0.45f, false },
                { "Angle %.0f\xC2\xB0", ParamKind::Float, 0.0f, 360.0f, 90.0f, false },
            }, true, "", starDistance },
            { "Rhombus", ModeKind::Shape, R"(
                d = abs(p.x) + abs(p.y) - 1.0;
            )", {}, true, "", rhombusDistance },
            { "Rounded rectangle", ModeKind::Shape, R"(
                vec2 inner = max(abs(p) - (1.0 - S0), 0.0);
                d = length(inner) - S0;
            )", {
                { "Corner %.2f", ParamKind::Float, 0.05f, 1.0f, 0.3f, false },
            }, true, "", roundedDistance },
            { "Cross", ModeKind::Shape, R"(
                d = min(max(abs(p.x) - S0, abs(p.y) - 1.0), max(abs(p.y) - S0, abs(p.x) - 1.0));
            )", {
                { "Arm %.2f", ParamKind::Float, 0.05f, 1.0f, 0.35f, false },
            }, true, "", crossDistance },
            { "Trapezoid", ModeKind::Shape, R"(
                float halfWidth = mix(1.0, S0, p.y * 0.5 + 0.5);
                d = max(abs(p.y) - 1.0, abs(p.x) - halfWidth);
            )", {
                { "Top width %.2f", ParamKind::Float, 0.05f, 1.0f, 0.4f, false },
            }, true, R"(
                float halfWidth = max(mix(1.0, S0, p.y * 0.5 + 0.5), 1e-4);
                md = max(abs(p.y) - 1.0, abs(p.x) - halfWidth);
                q = vec2(p.x / halfWidth, p.y);
            )", trapezoidDistance },
            { "Ring", ModeKind::Shape, R"(
                float radius = length(p);
                d = max(radius - 1.0, S0 - radius);
            )", {
                { "Hole %.2f", ParamKind::Float, 0.05f, 0.95f, 0.5f, false },
            }, true, R"(
                float radius = length(p);
                md = max(radius - 1.0, S0 - radius);
                q = vec2(atan(p.y, p.x) / PI, ((radius - S0) / max(1.0 - S0, 1e-4)) * 2.0 - 1.0);
            )", ringDistance },
            { "Source", ModeKind::Source, "", {
                { "Amount %.2f", ParamKind::Float, 0.0f, Config::SOURCE_MAX_AMOUNT, 1.0f, false },
            } },
            { "Log-Polar", ModeKind::Geometry, R"(
                float maxRadius = 0.5 * min(aspect, 1.0);
                float lowRadius = maxRadius * P0;
                float radius = lowRadius * exp(uv.x * log(maxRadius / lowRadius));
                float angle = uv.y * TAU;
                uv = toTexture(radius * vec2(cos(angle), sin(angle)));
            )", {
                { "Min radius %.4f", ParamKind::Float, Config::LOG_POLAR_MIN_RADIUS_LOW, Config::LOG_POLAR_MIN_RADIUS_HIGH, Config::LOG_POLAR_MIN_RADIUS, true },
            }, true },
            { "Julia (z\xC2\xB2 + c)", ModeKind::Geometry, R"(
                vec2 span = vec2(aspect, 1.0) * JULIA_HEIGHT;
                vec2 z = (uv - 0.5) * span;
                z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + vec2(P0, P1);
                uv = z / span + 0.5;
            )", {
                { "c real %.4f", ParamKind::Float, -Config::JULIA_C_LIMIT, Config::JULIA_C_LIMIT, Config::JULIA_DEFAULT_REAL, false },
                { "c imag %.4f", ParamKind::Float, -Config::JULIA_C_LIMIT, Config::JULIA_C_LIMIT, Config::JULIA_DEFAULT_IMAG, false },
            }, true },
            { "Droste", ModeKind::Geometry, R"(
                vec2 z = plane(uv);
                float turn = fract(atan(z.y, z.x) / TAU);
                float rings = log(max(length(z), 1e-6)) / log(P0);
                uv = vec2(fract(rings + P1 * turn), turn);
            )", {
                { "Zoom per ring %.2fx", ParamKind::Float, Config::DROSTE_MIN_ZOOM, Config::DROSTE_MAX_ZOOM, Config::DROSTE_DEFAULT_ZOOM, true },
                { "Spiral arms %d", ParamKind::Integer, -Config::DROSTE_ARM_LIMIT, Config::DROSTE_ARM_LIMIT, Config::DROSTE_DEFAULT_ARMS, false },
            }, true },
            { "Power (z\xE2\x81\xBF)", ModeKind::Geometry, R"(
                vec2 z = plane(uv);
                float radius = pow(max(length(z), 1e-6) * 2.0, P0) * 0.5;
                float angle = atan(z.y, z.x) * P0;
                uv = toTexture(radius * vec2(cos(angle), sin(angle)));
            )", {
                { "Power %.2f", ParamKind::Float, -Config::POWER_LIMIT, Config::POWER_LIMIT, Config::POWER_DEFAULT, false },
            }, true },
            { "Kaleidoscope", ModeKind::Geometry, R"(
                vec2 z = plane(uv);
                float offset = P1 * PI / 180.0;
                float wedge = TAU / P0;
                float angle = mod(atan(z.y, z.x) - offset, wedge);
                angle = min(angle, wedge - angle) + offset;
                uv = toTexture(length(z) * vec2(cos(angle), sin(angle)));
            )", {
                { "Segments %d", ParamKind::Integer, Config::KALEIDOSCOPE_MIN_SEGMENTS, Config::KALEIDOSCOPE_MAX_SEGMENTS, Config::KALEIDOSCOPE_DEFAULT_SEGMENTS, false },
                { "Wedge angle %.1f\xC2\xB0", ParamKind::Float, 0.0f, 360.0f, 0.0f, false },
            }, true },
            { "Inversion (1/z)", ModeKind::Geometry, R"(
                vec2 z = plane(uv);
                float lengthSquared = max(dot(z, z), 1e-8);
                uv = toTexture(z * (P0 * P0 * 0.25) / lengthSquared);
            )", {
                { "Circle radius %.3f", ParamKind::Float, Config::INVERSION_MIN_RADIUS, Config::INVERSION_MAX_RADIUS, Config::INVERSION_DEFAULT_RADIUS, true },
            }, true },
            { "Swirl", ModeKind::Geometry, R"(
                vec2 z = plane(uv);
                float extent = max(P1 * 0.5, 1e-6);
                float falloff = max(1.0 - length(z) / extent, 0.0);
                float angle = P0 * TAU * falloff * falloff;
                float sine = sin(angle);
                float cosine = cos(angle);
                uv = toTexture(vec2(z.x * cosine - z.y * sine, z.x * sine + z.y * cosine));
            )", {
                { "Swirl %.2f turns", ParamKind::Float, -Config::SWIRL_STRENGTH_LIMIT, Config::SWIRL_STRENGTH_LIMIT, Config::SWIRL_DEFAULT_STRENGTH, false },
                { "Swirl radius %.2f", ParamKind::Float, Config::SWIRL_MIN_RADIUS, Config::SWIRL_MAX_RADIUS, Config::SWIRL_DEFAULT_RADIUS, false },
            }, true },
            { "Tile / Mirror", ModeKind::Geometry, R"(
                vec2 scaled = uv * P0;
                vec2 cell = fract(scaled);
                vec2 flipped = mod(floor(scaled), 2.0) * step(0.5, P1);
                uv = mix(cell, 1.0 - cell, flipped);
            )", {
                { "Tiles %d", ParamKind::Integer, Config::TILE_MIN_COUNT, Config::TILE_MAX_COUNT, Config::TILE_DEFAULT_COUNT, false },
                { "Mirror tiles", ParamKind::Boolean, 0.0f, 1.0f, 1.0f, false },
            }, true },
            { "Sharpen", ModeKind::Sampling, R"(
                vec2 texel = 1.0 / vec2(textureSize(tex, 0));
                vec4 middle = readCanvas(uv);
                vec4 neighbours = readCanvas(uv + vec2(texel.x, 0.0)) + readCanvas(uv - vec2(texel.x, 0.0))
                    + readCanvas(uv + vec2(0.0, texel.y)) + readCanvas(uv - vec2(0.0, texel.y));
                layer = P0 * (middle * 4.0 - neighbours) * 0.25;
            )", {
                { "Sharpen %.2f", ParamKind::Float, 0.0f, Config::SHARPEN_STRENGTH_LIMIT, Config::SHARPEN_DEFAULT_STRENGTH, false },
            } },
            { "Hue shift", ModeKind::Color, R"(
                vec3 hsv = rgbToHsv(accum.rgb);
                hsv.x = fract(hsv.x + P0);
                accum = vec4(hsvToRgb(hsv), accum.a);
            )", {
                { "Hue %.3f turns", ParamKind::Float, -Config::HUE_SHIFT_LIMIT, Config::HUE_SHIFT_LIMIT, Config::HUE_SHIFT_DEFAULT, false },
            } },
            { "M\xC3\xB6" "bius", ModeKind::Geometry, R"(
                vec2 z = plane(uv);
                vec2 numerator = z + vec2(P0, P1);
                vec2 denominator = vec2(P2 * z.x - P3 * z.y, P2 * z.y + P3 * z.x) + vec2(1.0, 0.0);
                float lengthSquared = max(dot(denominator, denominator), 1e-8);
                vec2 w = vec2(dot(numerator, denominator), numerator.y * denominator.x - numerator.x * denominator.y) / lengthSquared;
                uv = toTexture(w);
            )", {
                { "shift real %.3f", ParamKind::Float, -Config::MOBIUS_B_LIMIT, Config::MOBIUS_B_LIMIT, Config::MOBIUS_DEFAULT_B_REAL, false },
                { "shift imag %.3f", ParamKind::Float, -Config::MOBIUS_B_LIMIT, Config::MOBIUS_B_LIMIT, Config::MOBIUS_DEFAULT_B_IMAG, false },
                { "bend real %.3f", ParamKind::Float, -Config::MOBIUS_C_LIMIT, Config::MOBIUS_C_LIMIT, Config::MOBIUS_DEFAULT_C_REAL, false },
                { "bend imag %.3f", ParamKind::Float, -Config::MOBIUS_C_LIMIT, Config::MOBIUS_C_LIMIT, Config::MOBIUS_DEFAULT_C_IMAG, false },
            }, true },
            { "Invert", ModeKind::Color, R"(
                vec3 straight = accum.a > 0.0 ? accum.rgb / accum.a : accum.rgb;
                vec3 flipped;
                if (P2 > 0.5) {
                    vec3 hsv = rgbToHsv(straight);
                    hsv.x = fract(hsv.x + 0.5 * P0);
                    flipped = hsvToRgb(hsv);
                } else {
                    flipped = mix(straight, vec3(1.0) - straight, P0);
                }
                flipped = clamp((flipped - 0.5) * P1 + 0.5, 0.0, 1.0);
                accum = vec4(flipped * accum.a, accum.a);
            )", {
                { "Invert %.2f", ParamKind::Float, 0.0f, 1.0f, Config::INVERT_DEFAULT_AMOUNT, false },
                { "Contrast %.2f", ParamKind::Float, Config::INVERT_MIN_CONTRAST, Config::INVERT_MAX_CONTRAST, Config::INVERT_DEFAULT_CONTRAST, false },
                { "Hue flip", ParamKind::Boolean, 0.0f, 1.0f, 1.0f, false },
            } },
            { "Chromatic split", ModeKind::Sampling, R"(
                vec2 offset = uv - 0.5;
                vec4 source = readCanvas(uv);
                float red = readCanvas(0.5 + offset * (1.0 - P0)).r;
                float blue = readCanvas(0.5 + offset * (1.0 + P0)).b;
                layer = vec4(red - source.r, 0.0, blue - source.b, 0.0);
            )", {
                { "Split %.4f", ParamKind::Float, -Config::CHROMATIC_SPLIT_LIMIT, Config::CHROMATIC_SPLIT_LIMIT, Config::CHROMATIC_DEFAULT_SPLIT, false },
            } },
            { "Newton (z\xE2\x81\xBF - 1)", ModeKind::Geometry, R"(
                vec2 span = vec2(aspect, 1.0) * NEWTON_HEIGHT;
                vec2 z = (uv - 0.5) * span;
                float radius = max(length(z), 1e-6);
                float angle = atan(z.y, z.x);
                vec2 zPower = pow(radius, P0) * vec2(cos(P0 * angle), sin(P0 * angle)) - vec2(1.0, 0.0);
                float derivativeAngle = (P0 - 1.0) * angle;
                vec2 derivative = P0 * pow(radius, P0 - 1.0) * vec2(cos(derivativeAngle), sin(derivativeAngle));
                float lengthSquared = max(dot(derivative, derivative), 1e-8);
                vec2 quotient = vec2(dot(zPower, derivative), zPower.y * derivative.x - zPower.x * derivative.y) / lengthSquared;
                uv = (z - P1 * quotient) / span + 0.5;
            )", {
                { "Roots %d", ParamKind::Integer, Config::NEWTON_MIN_ORDER, Config::NEWTON_MAX_ORDER, Config::NEWTON_DEFAULT_ORDER, false },
                { "Step %.2f", ParamKind::Float, Config::NEWTON_MIN_STEP, Config::NEWTON_MAX_STEP, Config::NEWTON_DEFAULT_STEP, false },
            }, true },
            { "Shear", ModeKind::Geometry, R"(
                vec2 z = plane(uv);
                uv = toTexture(vec2(z.x + P0 * z.y, z.y + P1 * z.x));
            )", {
                { "Shear X %.3f", ParamKind::Float, -Config::SHEAR_LIMIT, Config::SHEAR_LIMIT, 0.2f, false },
                { "Shear Y %.3f", ParamKind::Float, -Config::SHEAR_LIMIT, Config::SHEAR_LIMIT, 0.0f, false },
            }, true },
        });
        return modes;
    }

    std::vector<int> sortedModes(const std::vector<ModeKind>& kinds) {
        std::vector<int> modes;
        for (int mode = 0; mode < static_cast<int>(table().size()); ++mode) {
            if (std::find(kinds.begin(), kinds.end(), table()[mode].kind) != kinds.end()) {
                modes.push_back(mode);
            }
        }
        std::sort(modes.begin(), modes.end(), [](int left, int right) {
            return std::string(table()[left].name) < std::string(table()[right].name);
        });
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

    const std::vector<BrowserGroup>& browserGroups() {
        static const std::vector<BrowserGroup> groups = {
            { "Shape", sortedModes({ ModeKind::Shape }) },
            { "Aim", sortedModes({ ModeKind::Geometry }) },
            { "Source", sortedModes({ ModeKind::Source }) },
            { "Effects", sortedModes({ ModeKind::Sampling, ModeKind::Color }) },
        };
        return groups;
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

    float clampParam(int mode, int param, float value) {
        const ModeInfo& info = get(mode);
        if (param < 0 || param >= static_cast<int>(info.params.size())) return value;

        const ModeParam& slot = info.params[param];
        value = std::clamp(value, slot.minimum, slot.maximum);
        if (slot.kind == ParamKind::Integer) value = std::round(value);
        if (slot.kind == ParamKind::Boolean) value = value > 0.5f ? 1.0f : 0.0f;
        return value;
    }

    float shapeDistance(const ModeEntry& entry, float x, float y) {
        const ModeInfo& info = get(entry.mode);
        if (info.distance == nullptr) return std::max(std::abs(x), std::abs(y)) - 1.0f;
        return info.distance(entry.params, x, y);
    }

    bool contains(const std::vector<ModeEntry>& stack, float x, float y) {
        for (const ModeEntry& entry : stack) {
            if (entry.muted || get(entry.mode).kind != ModeKind::Shape) continue;
            return shapeDistance(entry, x, y) <= 0.0f;
        }
        return std::max(std::abs(x), std::abs(y)) <= 1.0f;
    }

    bool reads(int mode) {
        const ModeKind kind = get(mode).kind;
        return kind == ModeKind::Source || kind == ModeKind::Sampling;
    }

    int insertIndex(const std::vector<ModeEntry>& stack, int mode) {
        if (get(mode).kind == ModeKind::Shape) return 0;
        if (!get(mode).insertBeforeSource) return static_cast<int>(stack.size());

        for (int index = 0; index < static_cast<int>(stack.size()); ++index) {
            if (get(stack[index].mode).kind == ModeKind::Source) return index;
        }
        return static_cast<int>(stack.size());
    }

    int sourceMode() {
        for (int mode = 0; mode < count(); ++mode) {
            if (get(mode).kind == ModeKind::Source) return mode;
        }
        return 0;
    }

    ModeEntry makeEntry(int mode) {
        ModeEntry entry;
        entry.mode = std::clamp(mode, 0, count() - 1);
        const ModeInfo& info = get(entry.mode);
        for (int param = 0; param < static_cast<int>(info.params.size()); ++param) {
            entry.params[param] = info.params[param].defaultValue;
        }
        return entry;
    }

    std::string buildFragmentShader() {
        const std::vector<ModeInfo>& modes = table();
        std::string source = replaceAll(SHADER_PRELUDE, "PARAM_COUNT", std::to_string(MAX_STACK * MAX_PARAMS));
        source = replaceAll(source, "MAX_STACK", std::to_string(MAX_STACK));
        source = replaceAll(source, "MAX_PARAMS", std::to_string(MAX_PARAMS));

        for (int mode = 0; mode < count(); ++mode) {
            const ModeInfo& info = modes[mode];
            std::string body = replaceAll(info.glsl, "JULIA_HEIGHT", number(Config::JULIA_VIEW_HEIGHT));
            body = replaceAll(body, "NEWTON_HEIGHT", number(Config::NEWTON_VIEW_HEIGHT));

            const std::string guard = info.kind == ModeKind::Shape ? "" : "overlay == 0 && ";
            source += "                if (" + guard + "mode == " + std::to_string(mode) + ") {\n";
            if (info.kind == ModeKind::Shape) {
                const int own = static_cast<int>(info.params.size()) - (info.fitGlsl[0] == '\0' ? 2 : 1);
                std::string fit = info.fitGlsl[0] == '\0' ? std::string(R"(
                    float radius = length(p);
                    vec2 direction = radius > 1e-6 ? p / radius : vec2(1.0, 0.0);
                    float edge = boundaryRadius(direction);
                    md = radius / edge - 1.0;
                    q = discToSquare(p / edge, ROUND);
                )") : std::string(info.fitGlsl);
                fit = replaceAll(fit, "ROUND", "P" + std::to_string(own + 1));

                std::string setup = "                    shapeApplied = true;\n                    gShape = " + std::to_string(mode) + ";\n";
                for (int param = 0; param < 4; ++param) {
                    const std::string index = std::to_string(param);
                    setup += "                    gS" + index + " = P" + index + ";\n";
                    setup += "                    float S" + index + " = P" + index + ";\n";
                }

                source += "                    if (!shapeApplied) {\n" + setup +
                    "                    vec2 p = (uv - 0.5) * 2.0 * planeScale;\n"
                    "                    vec2 q = p;\n"
                    "                    float md = 0.0;\n" + fit +
                    "\n                    maskDistance = md;\n"
                    "                    if (P" + std::to_string(own) + " > 0.5) uv = q * 0.5 + 0.5;\n"
                    "                    }\n";
            } else if (info.kind == ModeKind::Source) {
                source += "                    accum += P0 * readCanvas(uv);\n";
            } else if (info.kind == ModeKind::Sampling) {
                source += "                    vec4 layer = vec4(0.0);\n" + body +
                    "\n                    accum += layer;\n";
            } else {
                source += body + "\n";
            }
            source += "                }\n";
        }

        source += R"(
            }

            float edge = max(fwidth(maskDistance), 1e-5);
            float coverage = clamp(0.5 - maskDistance / edge, 0.0, 1.0);

            if (overlay == 2) {
                fragColor = color * clamp((OUTLINE_HALF * edge - abs(maskDistance)) / edge + 0.5, 0.0, 1.0);
            } else if (overlay == 1) {
                fragColor = color * coverage;
            } else {
                fragColor = max(accum, vec4(0.0)) * color * coverage;
            }
        }
)";

        std::string branches;
        for (int mode = 0; mode < count(); ++mode) {
            const ModeInfo& info = modes[mode];
            if (info.kind != ModeKind::Shape) continue;
            branches += "            if (gShape == " + std::to_string(mode) + ") {\n" + info.glsl + "\n            }\n";
        }
        source = replaceAll(source, "SHAPE_BRANCHES", branches);
        source = replaceAll(source, "OUTLINE_HALF", number(Config::OUTLINE_THICKNESS * 0.5f));
        source = replaceAll(source, "SAMPLE_FILTER", Config::BICUBIC_SAMPLING ? "sampleBicubic" : "sampleBilinear");
        return source;
    }
}