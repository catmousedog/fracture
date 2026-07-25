/* ====================== FRACTURE ====================== */
#include "UI.hpp"

/* ======================== IMGUI ======================= */
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

////////////////////////////////////////////////////////////

UI::UI(Window* window)
    : _window(window)
{
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(_window->getWindow(), true);
}

////////////////////////////////////////////////////////////

UI::~UI()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

////////////////////////////////////////////////////////////

void UI::draw()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool show = true;
    ImGui::ShowDemoWindow(&show);

    ImGui::Render();
}