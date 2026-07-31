/* ====================== FRACTURE ====================== */
#include "Renderer.hpp"
#include "vulkan/VulkanContext.hpp"

/* ========================= STL ======================== */
#include <memory>

////////////////////////////////////////////////////////////

Renderer::Renderer(const Fractal* fractal, Window* window)
    : _fractal(fractal),
      _window(window),
      _vulkanContext(std::make_unique<VulkanContext>(_window))
{
    _vulkanContext->logInfo();
}

////////////////////////////////////////////////////////////

Renderer::~Renderer() = default;

////////////////////////////////////////////////////////////

void Renderer::draw()
{
    _vulkanContext->drawFrame();
}

////////////////////////////////////////////////////////////