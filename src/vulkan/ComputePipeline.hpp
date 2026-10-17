#pragma once

#include "vulkan/VulkanImage.hpp"

class ComputePipeline
{
    /* ======================== SETUP ======================= */
  public:
    ComputePipeline(VulkanCore& vulkanCore, VulkanImage& image);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    void readShader();

    /* ================== PUBLIC FUNCTIONS ================== */
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
    VulkanCore&  _vulkanCore;
    VulkanImage& _image;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool      _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;
    vk::raii::DescriptorSet       _descriptorSet       = nullptr;

    vk::raii::ShaderModule _shader = nullptr;
};