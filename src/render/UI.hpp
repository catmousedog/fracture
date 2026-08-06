#pragma once

#include "vulkan/CommandQueue.hpp"

class UI final
{
    /* ======================== SETUP ======================= */
  public:
    UI(class VulkanCore* vulkanCore, uint32_t framesInFlight);

    ~UI();

  private:
    void createDescriptorPool();

    void initImGUI();

    /* ==================== CAPABILITIES ==================== */
  public:
    void draw(CommandQueue& commandQueue);

    /* ====================== VARIABLES ===================== */
  private:
    class VulkanCore* _vulkanCore     = nullptr;
    uint32_t          _framesInFlight = 0;

    vk::raii::DescriptorPool _descriptorPool = nullptr;

    float _uiWidth = 200.0f;
};