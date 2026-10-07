#pragma once
#include <string>
#include <vector>

enum class ModeKind {
    Shape,
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

using ShapeDistance = float (*)(const float* params, float x, float y);

struct ModeInfo {
    const char* name;
    ModeKind kind;
    const char* glsl;
    std::vector<ModeParam> params;
    bool insertBeforeSource = false;
    const char* fitGlsl = "";
    ShapeDistance distance = nullptr;
};

namespace DisplayModes {
    constexpr int MAX_PARAMS = 6;
    constexpr int MAX_STACK = 32;
}

struct ModeEntry {
    int mode = 0;
    bool muted = false;
    float params[DisplayModes::MAX_PARAMS] = {};
};

struct BrowserGroup {
    const char* label;
    std::vector<int> modes;
};

namespace DisplayModes {
    const std::vector<ModeInfo>& all();
    const std::vector<BrowserGroup>& browserGroups();
    const ModeInfo& get(int mode);
    int count();
    int paramCount(int mode);
    float clampParam(int mode, int param, float value);
    int sourceMode();
    ModeEntry makeEntry(int mode);
    int insertIndex(const std::vector<ModeEntry>& stack, int mode);
    bool reads(int mode);
    float shapeDistance(const ModeEntry& entry, float x, float y);
    bool contains(const std::vector<ModeEntry>& stack, float x, float y);
    std::string buildFragmentShader();
}