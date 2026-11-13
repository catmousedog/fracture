#include "UI.hpp"

#include "util/Time.hpp"

#include "imgui.h"

#include <chrono>

////////////////////////////////////////////////////////////

namespace
{

constexpr float  collapseButtonSize = 32.0f;
constexpr ImVec4 bgColor            = ImVec4(.1f, .1f, .2f, .5f);
constexpr ImVec4 bgColorFocused     = ImVec4(.1f, .1f, .2f, 1.f);
constexpr ImVec4 bgColorHighlight   = ImVec4(.2f, .2f, .3f, 1.f);

template <typename T>
void enumListBox(const char* label, T& current)
{
    const float h =
        ImGui::GetTextLineHeightWithSpacing() * (T::all.size() + 0.25f) + ImGui::GetStyle().FramePadding.y * 2.0f;
    if (ImGui::BeginListBox(label, ImVec2(0, h)))
    {
        for (T t : T::all)
            if (ImGui::Selectable(t.toString().data(), t == current))
                current = t;
        ImGui::EndListBox();
    }
}

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

        // --- Fractal Settings --- //
        ImGui::Text("Fractal Settings");
        enumListBox("Fractal", _settings.fractal);
        ImGui::Separator();

        // --- Function Settings --- //
        ImGui::Text("Function Settings");
        enumListBox("Function", _settings.function);
        ImGui::Separator();

        // --- Filter Settings --- //
        ImGui::Text("Filter Settings");
        enumListBox("Filter", _settings.filter);
        ImGui::Separator();

        auto ns = std::chrono::nanoseconds{(long long)stats.ns};
        ImGui::Text("Render time: %s", Time::formatTime<std::chrono::microseconds>(ns).c_str());

        ImGui::DragScalar("Offset X", ImGuiDataType_Double, &_settings.offsetX, 0.01 * _settings.zoom);
        ImGui::DragScalar("Offset Y", ImGuiDataType_Double, &_settings.offsetY, 0.01 * _settings.zoom);

        double zMin = 1e-15, zMax = 4.0;
        ImGui::SliderScalar(
            "Zoom", ImGuiDataType_Double, &_settings.zoom, &zMin, &zMax, "%.3e", ImGuiSliderFlags_Logarithmic
        );

        int maxIter = _settings.maxIter;
        if (ImGui::InputInt("Max Iterations", &maxIter))
            _settings.maxIter = maxIter;

        if (ImGui::Button("Reset"))
            _settings = Backend::Settings{};

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

bool UI::captureMouse() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

////////////////////////////////////////////////////////////