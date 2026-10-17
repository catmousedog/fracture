#include "Fracture.hpp"
#include "backend/CPUBackend.hpp"
#include "backend/GPUBackend.hpp"
#include "vulkan/VulkanCore.hpp"

////////////////////////////////////////////////////////////

Fracture::Fracture()
    : _vulkanCore(&_window),
      _renderer(_vulkanCore, &_window),
      _ui(_settings)
{
    _prevBackendType = _settings.backendType = Backend::Type::GPU;
    switchBackend();
}

////////////////////////////////////////////////////////////

void Fracture::mainLoop()
{
    while (!_window.shouldClose())
    {
        _window.pollEvents();
        Window::Input input = _window.consumeInput();
        updateFractal(input);

        _renderer.beginUIFrame();         // prepare ImGui
        _ui.draw(_backend->getStats());   // create widgets
        updateBackend();                  // WIP: backend switching
        _backend->setSettings(_settings); // flag backend to recompute if needed
        _renderer.draw(*_backend);        // record graphics and ImGui (recompute if flagged)
    }
    _vulkanCore.waitIdle();
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
            _settings.fractal.maxIter *= iterFactor;
            _settings.fractal.maxIter = std::max(_settings.fractal.maxIter, 1.f);
        }
        else
        {
            float zoomFactor = (input.scroll < 0) ? 1.1f : 1.f / 1.1f;
            _settings.fractal.zoom *= zoomFactor;
        }
    }
    // DRAG
    _settings.fractal.offsetX -= input.dragX * _settings.fractal.zoom;
    _settings.fractal.offsetY -= input.dragY * _settings.fractal.zoom;
}

////////////////////////////////////////////////////////////

void Fracture::updateBackend()
{
    if (_settings.backendType != _prevBackendType)
    {
        _prevBackendType = _settings.backendType;
        _vulkanCore.waitIdle();
        switchBackend();
    }
}

////////////////////////////////////////////////////////////

void Fracture::switchBackend()
{
    switch (_settings.backendType)
    {
    case Backend::Type::GPU:
        _backend = std::make_unique<GPUBackend>(_vulkanCore, _renderer.getCommandQueue(), _renderer.getImage());
        break;
    case Backend::Type::CPU:
        _backend = std::make_unique<CPUBackend>(_vulkanCore, _renderer.getCommandQueue(), _renderer.getImage());
        break;
    }
}

////////////////////////////////////////////////////////////