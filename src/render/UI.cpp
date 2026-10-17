#include "UI.hpp"

#include "util/Time.hpp"

#include "imgui.h"

#include <chrono>
#include <utility>
#include <vector>

////////////////////////////////////////////////////////////

namespace
{

constexpr float  collapseButtonSize = 32.0f;
constexpr ImVec4 bgColor            = ImVec4(.1f, .1f, .2f, .5f);
constexpr ImVec4 bgColorFocused     = ImVec4(.1f, .1f, .2f, 1.f);
constexpr ImVec4 bgColorHighlight   = ImVec4(.2f, .2f, .3f, 1.f);

} // namespace

////////////////////////////////////////////////////////////

UI::UI(Backend::Settings& settings)
    : _settings(settings)
{
}

////////////////////////////////////////////////////////////

void UI::draw(const Backend::Stats& stats)
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

        /* ===================== UI - BEGIN ===================== */

        // --- Backend Settings --- //
        ImGui::Text("Backend Settings");
        ImGui::Separator();
        std::vector<const char*> items   = {"GPU"}; // WIP: reflection
        int                      current = std::to_underlying(_settings.backendType);
        if (ImGui::ListBox("Backend", &current, items.data(), items.size(), 1))
            _settings.backendType = static_cast<Backend::Type>(current);

        int scale = _settings.scale;
        if (ImGui::SliderInt("Scale", &scale, 1, 16))
            _settings.scale = scale;

        // --- Fractal Settings --- //
        ImGui::Text("Fractal Settings");
        ImGui::Separator();

        auto ns = std::chrono::nanoseconds{(long long)stats.ns};
        ImGui::Text("Render time: %s", Time::formatTime<std::chrono::microseconds>(ns).c_str());

        ImGui::DragFloat("Offset X", &_settings.fractal.offsetX, 0.01f * _settings.fractal.zoom);
        ImGui::DragFloat("Offset Y", &_settings.fractal.offsetY, 0.01f * _settings.fractal.zoom);

        ImGui::SliderFloat("Zoom", &_settings.fractal.zoom, 1e-5f, 4.0f, "%.6f", ImGuiSliderFlags_Logarithmic);

        int maxIter = _settings.fractal.maxIter;
        if (ImGui::InputInt("Max Iterations", &maxIter))
            _settings.fractal.maxIter = maxIter;

        ImGui::Checkbox("Freeze", &_settings.frozen);

        if (ImGui::Button("Reset"))
            _settings.fractal = Fractal{};

        /* ====================== UI - END ====================== */

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