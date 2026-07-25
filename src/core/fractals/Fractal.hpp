#pragma once

#include "util/Basics.hpp"

class Fractal
{

  public:
    Fractal();

    virtual ~Fractal();

    virtual constexpr string shaderFileName() const = 0;

    virtual vector<char> readShader() const;
};