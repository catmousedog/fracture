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

void UI::record(CommandQueue& commandQueue)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandQueue.getCommandBuffer());
}

////////////////////////////////////////////////////////////