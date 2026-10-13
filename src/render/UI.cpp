#include "UI.hpp"

#include "imgui.h"

////////////////////////////////////////////////////////////

namespace
{

constexpr float  collapseButtonSize = 32.0f;
constexpr ImVec4 bgColor            = ImVec4(.1f, .1f, .2f, .5f);
constexpr ImVec4 bgColorFocused     = ImVec4(.1f, .1f, .2f, 1.f);
constexpr ImVec4 bgColorHighlight   = ImVec4(.2f, .2f, .3f, 1.f);

} // namespace

////////////////////////////////////////////////////////////

UI::UI(Fractal& fractal)
    : _fractal(fractal)
{
}

////////////////////////////////////////////////////////////

void UI::draw()
{
    auto [width, height] = ImGui::GetIO().DisplaySize;

    ImVec2 mousePos = ImGui::GetMousePos();

    if (_collapsed)
    {
        bool mouseNearEdge = mousePos.x >= width - collapseButtonSize && //
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

            ImGui::PushStyleColor(ImGuiCol_Button, bgColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bgColorFocused);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, bgColorHighlight);

            if (ImGui::ArrowButton("##id", ImGuiDir_Left))
                _collapsed = false;

            ImGui::PopStyleColor(3);
            ImGui::End();
        }
    }
    else
    {
        bool mouseInUI = mousePos.x >= width - _uiWidth && //
                         mousePos.x <= width &&            //
                         mousePos.y >= 0 &&                //
                         mousePos.y <= height;             //

        ImGui::PushStyleColor(ImGuiCol_WindowBg, mouseInUI ? bgColorFocused : bgColor);

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
        if (ImGui::InputInt("Max Iterations", &maxIter))
            _fractal.maxIter = maxIter;

        if (ImGui::Button("Reset"))
            _fractal = Fractal{};

        ImGui::PopStyleColor();
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
        ImGui::PushStyleColor(ImGuiCol_Button, bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bgColorFocused);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, bgColorHighlight);

        if (ImGui::ArrowButton("##collapse", ImGuiDir_Right))
            _collapsed = true;

        ImGui::PopStyleColor(3);
        ImGui::End();
    }
}

////////////////////////////////////////////////////////////