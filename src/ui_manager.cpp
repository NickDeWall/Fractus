#include "ui_manager.h"
#include "screen.h"
#include "screen_manager.h"
#include "config.h"
#include <cmath>
#include <string>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

namespace {
    const char* const DISPLAY_MODE_NAMES[] = { "Normal", "Prop", "Log-Polar", "Julia (z\xC2\xB2 + c)", "Droste", "Power (z\xE2\x81\xBF)", "Kaleidoscope", "Inversion (1/z)" };
    static_assert(sizeof(DISPLAY_MODE_NAMES) / sizeof(DISPLAY_MODE_NAMES[0]) == static_cast<size_t>(DisplayMode::Count),
        "Every display mode needs a name");

    float wrapDegrees(float degrees) {
        return degrees - 360.0f * std::floor(degrees / 360.0f);
    }

    bool rotationDial(const char* id, float& degrees, float radius) {
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(id, ImVec2(radius * 2.0f, radius * 2.0f));
        const bool active = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 center(origin.x + radius, origin.y + radius);

        bool changed = false;
        if (active) {
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const float dx = mouse.x - center.x;
            const float dy = mouse.y - center.y;
            if (dx * dx + dy * dy > 4.0f) {
                const float angle = wrapDegrees(std::atan2(dx, -dy) * 180.0f / static_cast<float>(Config::PI));
                if (angle != degrees) {
                    degrees = angle;
                    changed = true;
                }
            }
        }

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImU32 background = ImGui::GetColorU32(active ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        const ImU32 handle = ImGui::GetColorU32(active ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
        const float radians = degrees * static_cast<float>(Config::PI) / 180.0f;
        const ImVec2 tip(center.x + std::sin(radians) * (radius - 5.0f), center.y - std::cos(radians) * (radius - 5.0f));

        draw->AddCircleFilled(center, radius, background);
        draw->AddLine(center, tip, handle, 2.0f);
        draw->AddCircleFilled(tip, 4.0f, handle);
        return changed;
    }
}

UiManager::UiManager(SDL_Window* window, SDL_GLContext context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330");
}

UiManager::~UiManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void UiManager::processEvent(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}

bool UiManager::wantsMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool UiManager::wantsKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

void UiManager::toggleScreenMenu() {
    screenMenuOpen = !screenMenuOpen;
}

void UiManager::closeScreenMenu() {
    screenMenuOpen = false;
}

bool UiManager::isScreenMenuOpen() const {
    return screenMenuOpen;
}

void UiManager::render(ScreenManager& screenManager) {
    const std::vector<Screen*> selected = screenManager.getSelectedScreens();
    if (selected.empty()) {
        screenMenuOpen = false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (screenMenuOpen) {
        drawScreenMenu(screenManager, selected);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UiManager::drawScreenMenu(ScreenManager& screenManager, const std::vector<Screen*>& selected) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float padding = 10.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - padding, viewport->WorkPos.y + viewport->WorkSize.y - padding),
        ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("Screen", &screenMenuOpen, flags)) {
        Screen* primary = selected.front();

        if (selected.size() == 1) {
            ImGui::Text("Screen #%d", primary->getId());
        } else {
            ImGui::Text("%d screens selected", static_cast<int>(selected.size()));
        }
        ImGui::Separator();

        drawDisplayModeCombo(selected);
        if (selected.front()->getDisplayMode() == DisplayMode::LogPolar) {
            drawLogPolarSlider(selected);
        } else if (selected.front()->getDisplayMode() == DisplayMode::Julia) {
            drawJuliaSliders(selected);
        } else if (selected.front()->getDisplayMode() == DisplayMode::Droste) {
            drawDrosteSliders(selected);
        } else if (selected.front()->getDisplayMode() == DisplayMode::Power) {
            drawPowerSlider(selected);
        } else if (selected.front()->getDisplayMode() == DisplayMode::Kaleidoscope) {
            drawKaleidoscopeSliders(selected);
        } else if (selected.front()->getDisplayMode() == DisplayMode::Inversion) {
            drawInversionSlider(selected);
        }
        ImGui::Spacing();
        drawSizeSlider(screenManager, selected);
        ImGui::Spacing();
        drawRotationControl(selected);
        ImGui::Spacing();
        drawRateSlider(selected);
        drawDelaySlider(selected);
        ImGui::Spacing();

        float hsva[4] = { primary->getHue(), primary->getSaturation(), primary->getValue(), primary->getAlpha() };
        const ImGuiColorEditFlags colorFlags = ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_Float;

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::ColorPicker4("##color", hsva, colorFlags)) {
            for (Screen* screen : selected) {
                screen->setHue(hsva[0]);
                screen->setSaturation(hsva[1]);
                screen->setValue(hsva[2]);
                screen->setAlpha(hsva[3]);
            }
        }
    }
    ImGui::End();
}

void UiManager::drawSizeSlider(ScreenManager& screenManager, const std::vector<Screen*>& selected) {
    float percent = selected.front()->getTargetWidth() * 100.0f / screenManager.getWidth();

    if (!sizeEditing) {
        sizeBasePercent = percent;
        sizeBaseline.clear();
        for (Screen* screen : selected) {
            sizeBaseline[screen->getId()] = { screen->getTargetWidth(), screen->getTargetHeight() };
        }
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool changed = ImGui::SliderFloat("##size", &percent, 1.0f, Config::MAX_SCREEN_RATIO * 100.0f,
        "Size %.1f%%", ImGuiSliderFlags_AlwaysClamp);
    sizeEditing = ImGui::IsItemActive();

    if (!changed || sizeBasePercent <= 0.0f) return;

    const float ratio = percent / sizeBasePercent;
    for (Screen* screen : selected) {
        const auto base = sizeBaseline.find(screen->getId());
        if (base == sizeBaseline.end()) continue;

        const SDL_FPoint size = screenManager.clampSize(base->second.x * ratio, base->second.y * ratio);
        screen->setWidth(size.x);
        screen->setHeight(size.y);
    }
}

void UiManager::drawRotationControl(const std::vector<Screen*>& selected) {
    float angle = wrapDegrees(selected.front()->getRotation());

    if (!rotationEditing) {
        rotationBaseAngle = angle;
        rotationBaseline.clear();
        for (Screen* screen : selected) {
            rotationBaseline[screen->getId()] = screen->getRotation();
        }
    }

    bool changed = rotationDial("##rotationDial", angle, 28.0f);
    bool active = ImGui::IsItemActive();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Rotation");
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::DragFloat("##rotation", &angle, 0.25f, 0.0f, 0.0f, "%.2f\xC2\xB0");
    active |= ImGui::IsItemActive();
    ImGui::EndGroup();

    rotationEditing = active;
    if (!changed) return;

    const float delta = angle - rotationBaseAngle;
    for (Screen* screen : selected) {
        const auto base = rotationBaseline.find(screen->getId());
        if (base == rotationBaseline.end()) continue;
        screen->setRotation(wrapDegrees(base->second + delta));
    }
}

void UiManager::drawRateSlider(const std::vector<Screen*>& selected) {
    float rate = selected.front()->getUpdateRate();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (!ImGui::SliderFloat("##rate", &rate, Config::MIN_UPDATE_RATE, Config::MAX_UPDATE_RATE, "Refresh %.1f Hz",
            ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {
        return;
    }

    for (Screen* screen : selected) {
        screen->setUpdateRate(rate);
    }
}

void UiManager::drawDelaySlider(const std::vector<Screen*>& selected) {
    float delayMs = selected.front()->getDelay() * 1000.0f;

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (!ImGui::SliderFloat("##delay", &delayMs, 0.0f, Config::MAX_DELAY * 1000.0f, "Delay %.0f ms",
            ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {
        return;
    }

    for (Screen* screen : selected) {
        screen->setDelay(delayMs / 1000.0f);
    }
}

void UiManager::drawDisplayModeCombo(const std::vector<Screen*>& selected) {
    const DisplayMode current = selected.front()->getDisplayMode();
    const std::string preview = std::string("Display: ") + DISPLAY_MODE_NAMES[static_cast<int>(current)];

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (!ImGui::BeginCombo("##displayMode", preview.c_str())) return;

    for (int i = 0; i < static_cast<int>(DisplayMode::Count); ++i) {
        const DisplayMode mode = static_cast<DisplayMode>(i);
        if (ImGui::Selectable(DISPLAY_MODE_NAMES[i], mode == current)) {
            for (Screen* screen : selected) {
                screen->setDisplayMode(mode);
            }
        }
    }
    ImGui::EndCombo();
}

void UiManager::drawLogPolarSlider(const std::vector<Screen*>& selected) {
    float radius = selected.front()->getLogPolarMinRadius();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (!ImGui::SliderFloat("##logPolarMinRadius", &radius, Config::LOG_POLAR_MIN_RADIUS_LOW, Config::LOG_POLAR_MIN_RADIUS_HIGH,
            "Min radius %.4f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {
        return;
    }

    for (Screen* screen : selected) {
        screen->setLogPolarMinRadius(radius);
    }
}

void UiManager::drawJuliaSliders(const std::vector<Screen*>& selected) {
    float real = selected.front()->getJuliaReal();
    float imag = selected.front()->getJuliaImag();
    const ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed = ImGui::SliderFloat("##juliaReal", &real, -Config::JULIA_C_LIMIT, Config::JULIA_C_LIMIT, "c real %.4f", flags);
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::SliderFloat("##juliaImag", &imag, -Config::JULIA_C_LIMIT, Config::JULIA_C_LIMIT, "c imag %.4f", flags);
    if (!changed) return;

    for (Screen* screen : selected) {
        screen->setJuliaC(real, imag);
    }
}

void UiManager::drawDrosteSliders(const std::vector<Screen*>& selected) {
    float zoom = selected.front()->getDrosteZoom();
    int arms = selected.front()->getDrosteArms();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderFloat("##drosteZoom", &zoom, Config::DROSTE_MIN_ZOOM, Config::DROSTE_MAX_ZOOM, "Zoom per ring %.2fx",
            ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {
        for (Screen* screen : selected) {
            screen->setDrosteZoom(zoom);
        }
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderInt("##drosteArms", &arms, -Config::DROSTE_ARM_LIMIT, Config::DROSTE_ARM_LIMIT, "Spiral arms %d",
            ImGuiSliderFlags_AlwaysClamp)) {
        for (Screen* screen : selected) {
            screen->setDrosteArms(arms);
        }
    }
}

void UiManager::drawPowerSlider(const std::vector<Screen*>& selected) {
    float power = selected.front()->getPower();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (!ImGui::SliderFloat("##power", &power, -Config::POWER_LIMIT, Config::POWER_LIMIT, "Power %.2f",
            ImGuiSliderFlags_AlwaysClamp)) {
        return;
    }

    for (Screen* screen : selected) {
        screen->setPower(power);
    }
}

void UiManager::drawKaleidoscopeSliders(const std::vector<Screen*>& selected) {
    int segments = selected.front()->getKaleidoscopeSegments();
    float angle = selected.front()->getKaleidoscopeAngle();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderInt("##kaleidoscopeSegments", &segments, Config::KALEIDOSCOPE_MIN_SEGMENTS, Config::KALEIDOSCOPE_MAX_SEGMENTS,
            "Segments %d", ImGuiSliderFlags_AlwaysClamp)) {
        for (Screen* screen : selected) {
            screen->setKaleidoscopeSegments(segments);
        }
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderFloat("##kaleidoscopeAngle", &angle, 0.0f, 360.0f, "Wedge angle %.1f\xC2\xB0", ImGuiSliderFlags_AlwaysClamp)) {
        for (Screen* screen : selected) {
            screen->setKaleidoscopeAngle(angle);
        }
    }
}

void UiManager::drawInversionSlider(const std::vector<Screen*>& selected) {
    float radius = selected.front()->getInversionRadius();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (!ImGui::SliderFloat("##inversionRadius", &radius, Config::INVERSION_MIN_RADIUS, Config::INVERSION_MAX_RADIUS,
            "Circle radius %.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {
        return;
    }

    for (Screen* screen : selected) {
        screen->setInversionRadius(radius);
    }
}