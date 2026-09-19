#include "ui_manager.h"
#include "screen.h"
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

void UiManager::render(const std::vector<Screen*>& selected) {
    if (selected.empty()) {
        screenMenuOpen = false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (screenMenuOpen) {
        drawScreenMenu(selected);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UiManager::drawScreenMenu(const std::vector<Screen*>& selected) {
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