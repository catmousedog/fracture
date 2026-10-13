#include "UI.hpp"

#include "imgui.h"

////////////////////////////////////////////////////////////

namespace
{

constexpr float collapseButtonSize = 32.0f;

} // namespace

////////////////////////////////////////////////////////////

UI::UI(Fractal& fractal)
    : _fractal(fractal)
{
}

////////////////////////////////////////////////////////////

// should be called in between the imgui backend functions
void UI::draw()
{
    auto [width, height] = ImGui::GetIO().DisplaySize;

    if (_collapsed)
    {
        ImVec2 mousePos      = ImGui::GetMousePos();
        bool   mouseNearEdge = mousePos.x >= width - collapseButtonSize && //
                               mousePos.x <= width &&                      //
                               mousePos.y >= 0 &&                          //
                               mousePos.y <= height;                       //
        if (mouseNearEdge)
        {
            ImGui::SetNextWindowPos(ImVec2(width - collapseButtonSize, (height - collapseButtonSize) / 2.f));
            ImGui::SetNextWindowSize(ImVec2(collapseButtonSize, collapseButtonSize));
            ImGui::Begin(
                "##ExpandArrow",
                nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground
            );

            if (ImGui::ArrowButton("##id", ImGuiDir_Left))
                _collapsed = false;

            ImGui::End();
        }
    }
    else
    {
        ImGui::SetNextWindowPos(ImVec2(width - _uiWidth, 0));
        ImGui::SetNextWindowSize(ImVec2(_uiWidth, height), ImGuiCond_Once);
        ImGui::SetNextWindowSizeConstraints(ImVec2(50, height), ImVec2(FLT_MAX, height));
        ImGui::Begin(
            "Controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
        );
        _uiWidth = ImGui::GetWindowWidth();

        ImGui::Text("Fractal Settings");
        ImGui::Separator();

        ImGui::DragFloat("Offset X", &_fractal.offsetX, 0.01f * _fractal.zoom);
        ImGui::DragFloat("Offset Y", &_fractal.offsetY, 0.01f * _fractal.zoom);

        ImGui::SliderFloat("Zoom", &_fractal.zoom, 1e-5f, 4.0f, "%.6f", ImGuiSliderFlags_Logarithmic);

        int maxIter = _fractal.maxIter;
        ImGui::InputInt("Max Iterations", &maxIter);
        _fractal.maxIter = maxIter;

        if (ImGui::Button("Reset"))
            _fractal = Fractal{};

        ImGui::End();

        ImGui::SetNextWindowPos(
            ImVec2(width - _uiWidth - collapseButtonSize, height * 0.5f - collapseButtonSize * 0.5f)
        );
        ImGui::SetNextWindowSize(ImVec2(collapseButtonSize, collapseButtonSize));
        ImGui::Begin(
            "##CollapseArrow",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground
        );

        if (ImGui::ArrowButton("##collapse", ImGuiDir_Right))
            _collapsed = true;

        ImGui::End();
    }
}

////////////////////////////////////////////////////////////