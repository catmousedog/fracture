#pragma once

#include "vulkan/VulkanImage.hpp"
#include "vulkan/pipeline/Pipeline.hpp"

class ComputePipeline : public Pipeline
{
    /* ======================== SETUP ======================= */
  public:
    ComputePipeline(VulkanCore& vulkanCore, VulkanImage& image);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    void readShader();

    /* ==================== CAPABILITIES ==================== */
  public:
    void updatePipeline(uint32_t pushConstantSize);

    template <typename T>
    void pushConstants(CommandQueue& commandQueue, T& constants)
    {
        auto& commandBuffer = commandQueue.getCommandBuffer();
        commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(T), &constants);
    }

    /* ================== VIRTUAL FUNCTIONS ================= */

    void updateDescriptorSet();

    void bind(CommandQueue& commandQueue);

    /* ================= GETTERS AND SETTERS ================ */

    VulkanImage& getImage()
    {
        return _image;
    }

    /* ====================== VARIABLES ===================== */
  private:
    VulkanImage&           _image;
    vk::raii::ShaderModule _shader = nullptr;
};