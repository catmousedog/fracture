#pragma once

#include "backend/Backend.hpp"
#include "core/fractals/Fractal.hpp"
#include "vulkan/VertexBuffer.hpp"
#include "window/Window.hpp"

#include "vulkan/CommandQueue.hpp"
#include "vulkan/VulkanCore.hpp"
#include "vulkan/VulkanImage.hpp"
#include "vulkan/pipeline/GraphicsPipeline.hpp"

class Renderer
{
    /* ======================== SETUP ======================= */
  public:
    Renderer(VulkanCore& vulkanCore, const Fractal& fractal, Window* window);

    virtual ~Renderer();

  private:
    void initUI();

    /* ==================== CAPABILITIES ==================== */
  public:
    void beginUIFrame();

    void draw(Backend& backend, bool recompute);

    /* ================== HELPER FUNCTIONS ================== */
  private:
    void record(uint32_t imageIndex, Backend& backend, bool recompute);

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

  private:
    const Fractal& _fractal;

    Window* _window = nullptr;

    // --- Vulkan --- //
    VulkanCore&      _vulkanCore;
    CommandQueue     _commandQueue;
    VulkanImage      _vulkanImage;
    VertexBuffer     _vertexBuffer;
    GraphicsPipeline _graphicsPipeline;

    vk::raii::DescriptorPool _uiDescriptorPool = nullptr;

    bool _resized = false;
};
