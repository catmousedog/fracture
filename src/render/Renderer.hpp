#pragma once

#include "backend/Backend.hpp"
#include "util/Swappable.hpp"
#include "vulkan/GraphicsPipeline.hpp"
#include "vulkan/GraphicsQueue.hpp"
#include "vulkan/VertexBuffer.hpp"
#include "vulkan/VulkanCore.hpp"
#include "vulkan/VulkanImage.hpp"
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
    Swappable<VulkanImage>& getImages() { return _vulkanImages; }

    GraphicsQueue& getGraphicsQueue() { return _graphicsQueue; }

    /* ====================== VARIABLES ===================== */
  private:
    Window*                _window = nullptr;
    VulkanCore&            _vulkanCore;
    GraphicsQueue          _graphicsQueue;
    Swappable<VulkanImage> _vulkanImages;
    VertexBuffer           _vertexBuffer;
    GraphicsPipeline       _graphicsPipeline;

    vk::raii::DescriptorPool _uiDescriptorPool = nullptr;
};
