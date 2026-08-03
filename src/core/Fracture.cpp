#include "Fracture.hpp"
#include "core/fractals/Mandelbrot.hpp"

////////////////////////////////////////////////////////////

Fracture::Fracture()
    : _fractal(std::make_unique<Mandelbrot>()),
      _renderer(&_window, _fractal.get())

{
}

////////////////////////////////////////////////////////////

void Fracture::mainLoop()
{
    while (!_window.shouldClose())
    {
        _window.pollEvents();

        _renderer.draw();
    }
}

////////////////////////////////////////////////////////////