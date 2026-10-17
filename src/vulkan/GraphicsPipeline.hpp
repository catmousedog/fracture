#pragma once

#include "vulkan/VulkanImage.hpp"

#include <glm/glm.hpp>

class GraphicsPipeline
{
    /* ==================== NESTED CLASS ==================== */
  public:
    struct UniformBufferObject
    {
        alignas(8) glm::vec2 offset;
        alignas(4) float zoom;
    };

    /* ======================== SETUP ======================= */
  public:
    GraphicsPipeline(VulkanCore& vulkanCore, VulkanImage& image, uint32_t framesInFlight);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    void createPipeline();

    void createUniformBuffers();

  public:
    void updateDescriptorSet();

    void updateUniformBuffer(const CommandQueue& commandQueue);

    void bind(CommandQueue& commandQueue);

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore&  _vulkanCore;
    VulkanImage& _image;

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
    vector<UniformBuffer>           _uniformBuffers        = {};
    vector<vk::raii::DescriptorSet> _descriptorSets        = {};
    vector<UniformBufferObject>     _uniformBuffersObjects = {};
};