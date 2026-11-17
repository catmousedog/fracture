#include "Fracture.hpp"
#include "backend/GPUBackend.hpp"
#include "vulkan/VulkanCore.hpp"

////////////////////////////////////////////////////////////

Fracture::Fracture()
    : _vulkanCore(&_window),
      _renderer(_vulkanCore, &_window),
      _ui(_settings),
      _backend(std::make_unique<GPUBackend>(_vulkanCore, _renderer.getImage(), _renderer.getGraphicsQueue(), _settings))
{
    _prevSettings = _settings;
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

        _renderer.beginUIFrame();       // prepare ImGui
        _ui.draw(_backend->getStats()); // create widgets
        updateBackend();                // WIP: backend switching
        // _backend->setSettings(_settings); // flag backend to recompute if needed
        _renderer.draw(*_backend); // record graphics and ImGui (recompute if flagged)
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
            _settings.maxIter *= iterFactor;
            _settings.maxIter = std::max(_settings.maxIter, 1.f);
        }
        else
        {
            float zoomFactor = std::pow(1.1f, -input.scroll);
            float oldZoom    = _settings.zoom;
            float newZoom    = oldZoom * zoomFactor;

            _settings.offsetX += input.mouseX * (oldZoom - newZoom);
            _settings.offsetY += input.mouseY * (oldZoom - newZoom);
            _settings.zoom = newZoom;
        }
    }
    // DRAG
    _settings.offsetX -= input.dragX * _settings.zoom;
    _settings.offsetY -= input.dragY * _settings.zoom;
}

////////////////////////////////////////////////////////////

void Fracture::updateBackend()
{
    // Fractal type changed
    if (_settings.fractal != _prevSettings.fractal || _settings.function != _prevSettings.function ||
        _settings.filter != _prevSettings.filter)
    {
        _prevSettings.fractal  = _settings.fractal;
        _prevSettings.function = _settings.function;
        _prevSettings.filter   = _settings.filter;
        _backend->changeFractal();
    }

    // other Fractal settings changed
    if (_settings != _prevSettings)
    {
        _prevSettings = _settings;
        _backend->markDirty();
    }
}

////////////////////////////////////////////////////////////