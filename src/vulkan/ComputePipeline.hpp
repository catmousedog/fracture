#pragma once

#include "backend/Backend.hpp"
#include "vulkan/VulkanImage.hpp"

class ComputePipeline
{
    /* ==================== NESTED CLASS ==================== */
  private:
    struct Constants
    {
        alignas(2 * sizeof(real)) glm::vec<2, real> offset = {0.0, 0.0};
        alignas(sizeof(real)) real zoom                    = 1.0;
        alignas(4) int maxIter                             = 100;
    };

    /* ==================== NESTED CLASS ==================== */
  public:
    struct UniformBufferObject
    {
    };

    /* ======================== SETUP ======================= */

    ComputePipeline(VulkanCore& vulkanCore, VulkanImage& vulkanImage, const Backend::Settings& settings);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void updatePipeline();

    void createUniformBuffer();

    void pushConstants(VulkanQueue& computeQueue, const Backend::Settings& settings)
    {
        Constants constants;

        constants.offset  = glm::vec<2, real>(settings.offsetX, settings.offsetY);
        constants.zoom    = settings.zoom;
        constants.maxIter = (int)settings.maxIter;

        auto& commandBuffer = computeQueue.getCommandBuffer();
        commandBuffer.pushConstants(
            _pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(Constants), &constants
        );
    }

    void updateDescriptorSet();

    void readShader(const Backend::Settings& settings);

    void bind(VulkanQueue& computeQueue);

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore&  _vulkanCore;
    VulkanImage& _vulkanImage;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool      _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;
    vk::raii::DescriptorSet       _descriptorSet       = nullptr;

    // UBO
    vk::raii::Buffer       _buffer = nullptr;
    vk::raii::DeviceMemory _memory = nullptr;
    void*                  _mapped = nullptr;

    vk::raii::ShaderModule _shader = nullptr;
};