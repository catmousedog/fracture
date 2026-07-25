#include "Fracture.hpp"
#include "core/fractals/Mandelbrot.hpp"

////////////////////////////////////////////////////////////

Fracture::Fracture()
    : _fractal(std::make_unique<Mandelbrot>()),
      _ui(&_window),
      _renderer(_fractal.get(), &_window)
{
}

////////////////////////////////////////////////////////////

void Fracture::mainLoop()
{
    while (!_window.shouldClose())
    {
        _window.pollEvents();

        _ui.draw();
        _renderer.draw();
    }
}

////////////////////////////////////////////////////////////