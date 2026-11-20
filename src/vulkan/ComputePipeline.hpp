#pragma once

#include "backend/Backend.hpp"
#include "vulkan/VulkanImage.hpp"

class ComputePipeline
{
    /* ==================== NESTED CLASS ==================== */
  private:
    struct Constants
    {
        alignas(sizeof(real)) real offsetX = 0.;
        alignas(sizeof(real)) real offsetY = 0.;
        alignas(sizeof(real)) real zoom    = 1.;
        alignas(4) int maxIter             = 100;
    };

    /* ======================== SETUP ======================= */
  public:
    ComputePipeline(VulkanCore& vulkanCore, VulkanImage& vulkanImage, const Backend::Settings& settings);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    void createUniformBuffer(size_t uboSize);

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void updatePipeline();

    void updateDescriptorSet(size_t uboSize);

    void updateUniformBuffer(const Fractal& fractal);

    void bind(VulkanQueue& computeQueue);

    void readShader(const Backend::Settings& settings);

    void pushConstants(VulkanQueue& computeQueue, const Backend::Settings& settings)
    {
        Constants constants;

        constants.offsetX = settings.offsetX;
        constants.offsetY = settings.offsetY;
        constants.zoom    = settings.zoom;
        constants.maxIter = (int)settings.maxIter;

        auto& commandBuffer = computeQueue.getCommandBuffer();
        commandBuffer.pushConstants(
            _pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(Constants), &constants
        );
    }

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