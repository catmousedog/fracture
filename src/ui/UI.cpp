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

    // ImGuiIO& io = ImGui::GetIO();
    // ImGui::SetNextWindowPos(ImVec2(0, 0));
    // ImGui::SetNextWindowSize(io.DisplaySize);

    // ImGui::Begin(
    //     "MainWindow",
    //     nullptr,
    //     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
    //         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
    // );

    // float sidebarWidth = 300.0f;

    // ImGui::BeginChild("ImagePanel", ImVec2(-sidebarWidth, 0), true);
    // ImGui::Text("Image goes here");
    // // ImGui::Image((ImTextureID)my_texture_id, ImVec2(imgWidth, imgHeight));
    // ImGui::EndChild();

    // ImGui::SameLine();

    // ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), true);
    // ImGui::Text("Options");
    // ImGui::Separator();

    // static float value1 = 0.0f;
    // ImGui::SliderFloat("Slider", &value1, 0.0f, 1.0f);

    // static char buf[128] = "";
    // ImGui::InputText("Name", buf, IM_ARRAYSIZE(buf));

    // ImGui::EndChild();

    // ImGui::End();

    ImGui::Render();
}