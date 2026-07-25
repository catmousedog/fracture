#pragma once

#include "core/fractals/Fractal.hpp"
#include "window/Window.hpp"

#include <memory>

class Renderer
{
  public:
    Renderer(const Fractal* fractal, Window* window);

    virtual ~Renderer();

    void draw();

    bool shouldClose();
    void pollEvents();

  private:
    const Fractal* _fractal;

    Window*                              _window;
    std::unique_ptr<class VulkanContext> _vulkanContext;
};
