#pragma once

#include "util/Util.hpp"
#include "vulkan/VulkanImage.hpp"

class ComputePipeline
{
    /* ======================== SETUP ======================= */
  public:
    ComputePipeline(VulkanCore& vulkanCore, Swappable<VulkanImage>& images);

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

    Swappable<VulkanImage>& getImages() { return _images; }

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore&             _vulkanCore;
    Swappable<VulkanImage>& _images;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool             _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout        _descriptorSetLayout = nullptr;
    std::vector<vk::raii::DescriptorSet> _descriptorSet       = {};

    vk::raii::ShaderModule _shader = nullptr;
};