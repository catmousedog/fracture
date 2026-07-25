#pragma once

#include "window/Window.hpp"

class UI
{
  public:
    UI(Window* window);
    ~UI();

    void draw();

  private:
    Window* _window;
};