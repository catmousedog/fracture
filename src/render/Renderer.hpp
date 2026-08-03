#pragma once

#include "core/fractals/Fractal.hpp"
#include "render/UI.hpp"
#include "vulkan/VertexBuffer.hpp"
#include "window/Window.hpp"

#include "vulkan/CommandQueue.hpp"
#include "vulkan/VulkanCore.hpp"
#include "vulkan/VulkanImage.hpp"
#include "vulkan/pipeline/ComputePipeline.hpp"
#include "vulkan/pipeline/GraphicsPipeline.hpp"

class Renderer
{
    /* ======================== SETUP ======================= */
  public:
    Renderer(Window* window, const Fractal* fractal);

    virtual ~Renderer();

    /* ==================== CAPABILITIES ==================== */
    void draw();

    void record(uint32_t imageIndex);

    void resize();

  private:
    const Fractal* _fractal;

    Window* _window = nullptr;

    // --- Vulkan --- //
    VulkanCore       _vulkanCore;
    CommandQueue     _commandQueue;
    VertexBuffer     _vertexBuffer;
    VulkanImage      _vulkanImage;
    ComputePipeline  _computePipeline;
    GraphicsPipeline _graphicsPipeline;

    // --- UI --- //
    UI _ui;

    bool _dirty = true;
};
