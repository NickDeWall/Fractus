#include "ui_manager.h"
#include "screen.h"
#include "screen_manager.h"
#include "config.h"
#include "display_modes.h"
#include <cmath>
#include <string>

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
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 0.0f), ImVec2(300.0f, viewport->WorkSize.y * 0.8f));

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

        drawModeBrowser(selected);
        drawModeStack(selected);

        ImGui::Spacing();
        ImGui::SeparatorText("Screen");

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

void UiManager::drawModeBrowser(const std::vector<Screen*>& selected) {
    Screen* primary = selected.front();
    const bool full = static_cast<int>(primary->getStack().size()) >= DisplayModes::MAX_STACK;

    ImGui::BeginDisabled(full);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##modeBrowser", full ? "Stack is full" : "Add display mode...")) {
        for (int mode = 0; mode < DisplayModes::count(); ++mode) {
            if (!ImGui::Selectable(DisplayModes::get(mode).name)) continue;
            primary->addMode(mode);
            for (Screen* screen : selected) {
                screen->setStack(primary->getStack());
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
}

void UiManager::drawModeStack(const std::vector<Screen*>& selected) {
    Screen* primary = selected.front();
    const std::vector<ModeEntry>& stack = primary->getStack();

    if (stack.empty()) {
        ImGui::TextDisabled("Empty stack: this screen draws nothing.");
        return;
    }

    bool hasSource = false;
    for (const ModeEntry& entry : stack) {
        hasSource = hasSource || (!entry.muted && DisplayModes::reads(entry.mode));
    }
    if (!hasSource) {
        ImGui::TextDisabled("No Source: nothing is read from the canvas.");
    }

    int removeAt = -1;
    int muteAt = -1;
    int moveFrom = -1;
    int moveTo = -1;

    const int size = static_cast<int>(stack.size());
    std::vector<bool> sourceBefore(size, false);
    std::vector<bool> sourceAfter(size, false);
    for (int index = 1; index < size; ++index) {
        sourceBefore[index] = sourceBefore[index - 1] ||
            (!stack[index - 1].muted && DisplayModes::reads(stack[index - 1].mode));
    }
    for (int index = size - 2; index >= 0; --index) {
        sourceAfter[index] = sourceAfter[index + 1] ||
            (!stack[index + 1].muted && DisplayModes::reads(stack[index + 1].mode));
    }

    for (int index = 0; index < size; ++index) {
        const ModeEntry& entry = stack[index];
        const ModeInfo& info = DisplayModes::get(entry.mode);
        ImGui::PushID(index);

        const bool unused =
            (info.kind == ModeKind::Geometry && !sourceAfter[index]) ||
            (info.kind == ModeKind::Color && !sourceBefore[index]);
        const bool inactive = unused || entry.muted;
        const std::string label = std::to_string(index + 1) + ". " + info.name +
            (entry.muted ? " (muted)" : unused ? " (no effect)" : "");
        const float rowWidth = ImGui::GetContentRegionAvail().x;
        const float buttonWidth = ImGui::GetFrameHeight();

        ImGui::SetNextItemAllowOverlap();
        if (inactive) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        const bool open = ImGui::TreeNodeEx("##entry",
            (info.params.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0), "%s", label.c_str());
        if (inactive) ImGui::PopStyleColor();

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover)) {
            ImGui::SetDragDropPayload("MODE_ENTRY", &index, sizeof(int));
            ImGui::TextUnformatted(label.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODE_ENTRY")) {
                moveFrom = *static_cast<const int*>(payload->Data);
                moveTo = index;
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine(rowWidth - buttonWidth * 2.0f - ImGui::GetStyle().ItemSpacing.x);
        if (entry.muted) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        if (ImGui::SmallButton("m")) muteAt = index;
        if (entry.muted) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(entry.muted ? "Unmute" : "Mute");

        ImGui::SameLine(rowWidth - buttonWidth);
        if (ImGui::SmallButton("x")) removeAt = index;

        if (open && !info.params.empty()) {
            drawEntryParams(selected, index, entry);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (removeAt >= 0) {
        primary->removeMode(removeAt);
    } else if (muteAt >= 0) {
        primary->setMuted(muteAt, !stack[muteAt].muted);
    } else if (moveFrom >= 0) {
        primary->moveMode(moveFrom, moveTo);
    } else {
        return;
    }

    for (Screen* screen : selected) {
        screen->setStack(primary->getStack());
    }
}

void UiManager::drawEntryParams(const std::vector<Screen*>& selected, int index, const ModeEntry& entry) {
    const ModeInfo& info = DisplayModes::get(entry.mode);

    for (int param = 0; param < static_cast<int>(info.params.size()); ++param) {
        const ModeParam& slot = info.params[param];
        const std::string id = "##param" + std::to_string(param);
        float value = entry.params[param];
        bool changed = false;

        if (slot.kind == ParamKind::Boolean) {
            bool ticked = value > 0.5f;
            changed = ImGui::Checkbox((slot.label + id).c_str(), &ticked);
            value = ticked ? 1.0f : 0.0f;
        } else if (slot.kind == ParamKind::Integer) {
            int whole = static_cast<int>(value);
            ImGui::SetNextItemWidth(-FLT_MIN);
            changed = ImGui::SliderInt(id.c_str(), &whole, static_cast<int>(slot.minimum), static_cast<int>(slot.maximum),
                slot.label, ImGuiSliderFlags_AlwaysClamp);
            value = static_cast<float>(whole);
        } else {
            ImGui::SetNextItemWidth(-FLT_MIN);
            const ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp |
                (slot.logarithmic ? ImGuiSliderFlags_Logarithmic : 0);
            changed = ImGui::SliderFloat(id.c_str(), &value, slot.minimum, slot.maximum, slot.label, flags);
        }

        if (!changed) continue;
        for (Screen* screen : selected) {
            screen->setEntryParam(index, param, value);
        }
    }
}