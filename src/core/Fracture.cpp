#include "Fracture.hpp"
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
        if (!_ui.captureMouse())
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
            float iterFactor = std::pow(1.1f, input.scroll);
            _settings.fractal.maxIter *= iterFactor;
            _settings.fractal.maxIter = std::max(_settings.fractal.maxIter, 1.f);
        }
        else
        {
            float zoomFactor = std::pow(1.1f, -input.scroll);
            float oldZoom    = _settings.fractal.zoom;
            float newZoom    = oldZoom * zoomFactor;

            _settings.fractal.offsetX += input.mouseX * (oldZoom - newZoom);
            _settings.fractal.offsetY += input.mouseY * (oldZoom - newZoom);
            _settings.fractal.zoom = newZoom;
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
        _backend = std::make_unique<GPUBackend>(_vulkanCore, _renderer.getImages(), _renderer.getGraphicsQueue());
        break;
    }
}

////////////////////////////////////////////////////////////