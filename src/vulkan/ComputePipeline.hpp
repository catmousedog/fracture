#pragma once

#include "backend/Backend.hpp"
#include "vulkan/VulkanImage.hpp"

class ComputePipeline
{
    /* ==================== NESTED CLASS ==================== */
  private:
    template <typename R>
    struct Constants
    {
        alignas(2 * sizeof(R)) glm::vec<2, R> offset = {0.0, 0.0};
        alignas(sizeof(R)) R zoom                    = 1.0;
        alignas(4) int maxIter                       = 100;
    };

    /* ======================== SETUP ======================= */
  public:
    ComputePipeline(VulkanCore& vulkanCore, VulkanImage& vulkanImage, const Backend::Settings& settings);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    template <typename R>
    uint32_t constantSize() const
    {
        return sizeof(Constants<R>);
    }

    void updatePipeline(uint32_t pushConstantSize);

    template <typename R>
    void pushConstants(VulkanQueue& computeQueue, const Backend::Settings& settings)
    {
        Constants<R> constants;

        constants.offset  = glm::vec<2, double>(settings.offsetX, settings.offsetY);
        constants.zoom    = settings.zoom;
        constants.maxIter = (int)settings.maxIter;

        auto& commandBuffer = computeQueue.getCommandBuffer();
        commandBuffer.pushConstants(
            _pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(Constants<R>), &constants
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

    vk::raii::ShaderModule _shader = nullptr;
};