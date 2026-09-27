#pragma once
#include <string>
#include <vector>

enum class ModeKind {
    None,
    Prop,
    Geometry,
    Sampling,
    Color
};

enum class ParamKind {
    Float,
    Integer,
    Boolean
};

struct ModeParam {
    const char* label;
    ParamKind kind;
    float minimum;
    float maximum;
    float defaultValue;
    bool logarithmic;
};

struct ModeInfo {
    const char* name;
    ModeKind kind;
    const char* glsl;
    std::vector<ModeParam> params;
};

namespace DisplayModes {
    constexpr int MAX_PARAMS = 4;

    const std::vector<ModeInfo>& all();
    const ModeInfo& get(int mode);
    int count();
    int paramCount(int mode);
    bool isProp(int mode);
    float clampParam(int mode, int param, float value);
    std::string buildFragmentShader();
}