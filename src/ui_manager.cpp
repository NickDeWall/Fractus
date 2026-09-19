#include "ui_manager.h"
#include "screen.h"
#include "screen_manager.h"
#include "config.h"
#include <cmath>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

namespace {
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

        drawSizeSlider(screenManager, selected);
        ImGui::Spacing();
        drawRotationControl(selected);
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