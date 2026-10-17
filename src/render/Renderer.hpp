#pragma once

#include "backend/Backend.hpp"
#include "vulkan/CommandQueue.hpp"
#include "vulkan/GraphicsPipeline.hpp"
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
    void record(uint32_t imageIndex, Backend& backend);

    void resize(Backend& backend);

    /* ================= GETTERS AND SETTERS ================ */
  public:
    VulkanImage& getImage()
    {
        return _vulkanImage;
    }

    CommandQueue& getCommandQueue()
    {
        return _commandQueue;
    }

    /* ====================== VARIABLES ===================== */
  private:
    Window* _window = nullptr;

    // --- Vulkan --- //
    VulkanCore&      _vulkanCore;
    CommandQueue     _commandQueue;
    VulkanImage      _vulkanImage;
    VertexBuffer     _vertexBuffer;
    GraphicsPipeline _graphicsPipeline;

    vk::raii::DescriptorPool _uiDescriptorPool = nullptr;
};
