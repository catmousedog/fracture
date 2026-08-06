#include "UI.hpp"

#include "vulkan/VulkanCore.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

////////////////////////////////////////////////////////////

UI::UI(VulkanCore* vulkanCore, uint32_t framesInFlight)
    : _vulkanCore(vulkanCore),
      _framesInFlight(framesInFlight)
{
    createDescriptorPool();
    initImGUI();
}

////////////////////////////////////////////////////////////

void UI::createDescriptorPool()
{
    vk::DescriptorPoolSize poolSizes[] = {
        {vk::DescriptorType::eSampledImage, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE}, // 8
        {vk::DescriptorType::eSampler, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE}             // 2
    };
    vk::DescriptorPoolCreateInfo pool_info = {
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 10, // 8 + 2
        .poolSizeCount = 2,
        .pPoolSizes    = poolSizes
    };
    _descriptorPool = _vulkanCore->getDevice().createDescriptorPool(pool_info);
}

////////////////////////////////////////////////////////////

void UI::initImGUI()
{
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(_vulkanCore->getWindow()->getGLFWWindow(), true);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
        .colorAttachmentCount    = 1,                                      //
        .pColorAttachmentFormats = &_vulkanCore->getSurfaceFormat().format //
    };

    ImGui_ImplVulkan_InitInfo initInfo{
        .Instance            = *_vulkanCore->getInstance(),
        .PhysicalDevice      = *_vulkanCore->getPhysicalDevice(),
        .Device              = *_vulkanCore->getDevice(),
        .QueueFamily         = _vulkanCore->getFamilyIndex(),
        .Queue               = *_vulkanCore->getQueue(),
        .DescriptorPool      = *_descriptorPool,
        .MinImageCount       = _framesInFlight,
        .ImageCount          = _framesInFlight,
        .PipelineInfoMain    = {.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo},
        .UseDynamicRendering = true
    };

    ImGui_ImplVulkan_Init(&initInfo);
}

////////////////////////////////////////////////////////////

UI::~UI()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

////////////////////////////////////////////////////////////

void UI::draw(CommandQueue& commandQueue)
{
    auto [width, height] = _vulkanCore->getWindow()->getSize();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(width - _uiWidth, 0));
    ImGui::SetNextWindowSize(ImVec2(_uiWidth, height), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(50, height), ImVec2(FLT_MAX, height));
    ImGui::Begin(
        "Controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
    );
    _uiWidth = ImGui::GetWindowWidth();

    ImGui::Text("Fractal Settings");
    ImGui::Separator();

    // your widgets go here, in order, e.g.:
    // ImGui::SliderFloat("Zoom", &zoom, 0.1f, 10.0f);
    // ImGui::InputFloat2("Center", &centerX);
    // ImGui::Combo("Fractal Type", &fractalIndex, items, itemCount);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandQueue.getCommandBuffer());
}

////////////////////////////////////////////////////////////