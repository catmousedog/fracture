#include "Fracture.hpp"
#include "backend/GPUBackend.hpp"
#include "vulkan/VulkanCore.hpp"

////////////////////////////////////////////////////////////

Fracture::Fracture()
    : _vulkanCore(&_window),
      _renderer(_vulkanCore, _fractal, &_window),
      _backend(std::make_unique<GPUBackend>(_vulkanCore, _renderer.getCommandQueue(), _renderer.getImage()))
{
}

////////////////////////////////////////////////////////////

void Fracture::mainLoop()
{
    while (!_window.shouldClose())
    {
        _window.pollEvents();

        bool recompute = false;

        if (_fractal != _prevFractal)
        {
            recompute    = true;
            _prevFractal = _fractal;
        }

        _renderer.beginUIFrame();
        _ui.draw();
        _renderer.draw(*_backend, recompute);
    }
}

////////////////////////////////////////////////////////////