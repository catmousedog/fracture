#pragma once

#include "util/Swappable.hpp"
#include "vulkan/CommandQueue.hpp"

class VulkanImage
{
    /* ======================== SETUP ======================= */
  public:
    VulkanImage(VulkanCore& vulkanCore, CommandQueue& commandQueue);

  private:
    void createComputeImage();

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void recreateComputeImage();

    void createSampler();

    /* ================= GETTERS AND SETTERS ================ */
    vk::raii::Image&     getComputeImage() { return _computeImage; }
    vk::raii::ImageView& getComputeImageView() { return _computeImageView; }
    vk::raii::Sampler&   getSampler() { return _sampler; }
    vk::Extent2D         getExtent() { return _extent; }

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore&   _vulkanCore;
    CommandQueue& _commandQueue;

    // Compute image
    vk::raii::Image        _computeImage       = nullptr;
    vk::raii::DeviceMemory _computeImageMemory = nullptr;
    vk::raii::ImageView    _computeImageView   = nullptr;
    vk::raii::Sampler      _sampler            = nullptr;
    vk::Extent2D           _extent             = {0, 0};
};