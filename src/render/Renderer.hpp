#pragma once

#include "backend/Backend.hpp"
#include "vulkan/GraphicsPipeline.hpp"
#include "vulkan/VertexBuffer.hpp"
#include "vulkan/VulkanCore.hpp"
#include "vulkan/VulkanImage.hpp"
#include "vulkan/VulkanQueue.hpp"
#include "window/Window.hpp"

class Renderer
{
    /* ======================== SETUP ======================= */
  public:
    Renderer(VulkanCore& vulkanCore, Window* window);

    virtual ~Renderer();

  private:
    void initUI();

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void beginUIFrame();

    void draw(Backend& backend);

    /* ================== PRIVATE FUNCTIONS ================= */
  private:
    void record(uint32_t swapChainIndex, Backend& backend);

    void resize(Backend& backend);

    /* ================= GETTERS AND SETTERS ================ */
  public:
    VulkanImage& getImage() { return _vulkanImage; }

    VulkanQueue& getGraphicsQueue() { return _vulkanQueue; }

    /* ====================== VARIABLES ===================== */
  private:
    Window*          _window = nullptr;
    VulkanCore&      _vulkanCore;
    VulkanQueue      _vulkanQueue;
    VulkanImage      _vulkanImage;
    VertexBuffer     _vertexBuffer;
    GraphicsPipeline _graphicsPipeline;

    vk::raii::DescriptorPool _uiDescriptorPool = nullptr;
};
