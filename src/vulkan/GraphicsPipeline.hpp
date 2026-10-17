#pragma once

#include "util/Util.hpp"
#include "vulkan/VulkanImage.hpp"

#include <glm/glm.hpp>

class GraphicsPipeline
{
    /* ==================== NESTED CLASS ==================== */
  public:
    struct UniformBufferObject
    {
        alignas(8) glm::vec2 offset, prevOffset;
        alignas(4) float zoom, prevZoom;
        alignas(8) glm::uvec2 renderSize;
        alignas(4) float aspect;
    };

    /* ======================== SETUP ======================= */
  public:
    GraphicsPipeline(VulkanCore& vulkanCore, Swappable<VulkanImage>& images, uint32_t framesInFlight);

    /* ================== PUBLIC FUNCTIONS ================== */

    void updateDescriptorSet();

    void updateUniformBuffer(const CommandQueue& commandQueue, const UniformBufferObject&);

    void bind(CommandQueue& commandQueue);

    /* ================== HELPER FUNCTIONS ================== */
  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    void createPipeline();

    void createUniformBuffers();

    uint32_t imageIndex(uint32_t image, uint32_t frame) const { return image * _framesInFlight + frame; }

    /* ====================== VARIABLES ===================== */

    VulkanCore&             _vulkanCore;
    Swappable<VulkanImage>& _images;

    uint32_t _framesInFlight;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool      _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;

    struct UniformBuffer
    {
        vk::raii::Buffer       buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        void*                  mapped = nullptr;
    };
    // one for each frame-in-flight
    vector<UniformBuffer>           _uniformBuffers = {};
    vector<vk::raii::DescriptorSet> _descriptorSets = {};
};