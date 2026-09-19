#include "ui_manager.h"
#include "screen.h"
#include "screen_manager.h"
#include "config.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

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

        float hsva[4] = { primary->getHue(), primary->getSaturation(), primary->getValue(), primary->getAlpha() };
        const ImGuiColorEditFlags colorFlags = ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_AlphaPreviewHalf;

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