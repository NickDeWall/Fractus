#pragma once
#include <string>
#include <vector>

enum class ModeKind {
    Source,
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
    bool insertBeforeSource = false;
};

namespace DisplayModes {
    constexpr int MAX_PARAMS = 4;
    constexpr int MAX_STACK = 32;
}

struct ModeEntry {
    int mode = 0;
    bool muted = false;
    float params[DisplayModes::MAX_PARAMS] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

namespace DisplayModes {
    const std::vector<ModeInfo>& all();
    const ModeInfo& get(int mode);
    int count();
    int paramCount(int mode);
    float clampParam(int mode, int param, float value);
    ModeEntry makeEntry(int mode);
    int insertIndex(const std::vector<ModeEntry>& stack, int mode);
    bool reads(int mode);
    std::string buildFragmentShader();
}