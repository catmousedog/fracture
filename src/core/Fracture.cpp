#include "Fracture.hpp"
#include "backend/GPUBackend.hpp"
#include "vulkan/VulkanCore.hpp"

////////////////////////////////////////////////////////////

Fracture::Fracture()
    : _ui(_fractal),
      _vulkanCore(&_window),
      _renderer(_vulkanCore, &_window),
      _backend(std::make_unique<GPUBackend>(_vulkanCore, _renderer.getCommandQueue(), _renderer.getImage()))
{
}

////////////////////////////////////////////////////////////

void Fracture::mainLoop()
{
    while (!_window.shouldClose())
    {
        _window.pollEvents();

        Window::Input input = _window.consumeInput();
        updateFractal(input);

        _renderer.beginUIFrame();       // prepare ImGui
        _ui.draw();                     // create widgets
        _backend->setFractal(_fractal); // flag backend to recompute if needed
        _renderer.draw(*_backend);      // record graphics and ImGui (recompute if flagged)
    }
}

////////////////////////////////////////////////////////////

void Fracture::updateFractal(const Window::Input& input)
{
    // SCROLL
    if (input.scroll)
    {
        // CTRL
        if (input.ctrl)
        {
            float iterFactor = (input.scroll < 0) ? 1 / 1.1f : 1.1f;
            _fractal.maxIter *= iterFactor;
            _fractal.maxIter = std::max(_fractal.maxIter, 1.f);
        }
        else
        {
            float zoomFactor = (input.scroll < 0) ? 1.1f : 1.f / 1.1f;
            _fractal.zoom *= zoomFactor;
        }
    }
    // DRAG
    _fractal.offsetX -= input.dragX * _fractal.zoom;
    _fractal.offsetY -= input.dragY * _fractal.zoom;
}

////////////////////////////////////////////////////////////