#pragma once

#include "util/Basics.hpp"
#include "vulkan/PushConstants.hpp"

class Fractal
{

    /* ======================== SETUP ======================= */
  public:
    Fractal();

    virtual ~Fractal();

    /* ==================== CAPABILITIES ==================== */

    virtual const PushConstants& getPushConstants() const = 0;

    virtual constexpr string shaderFileName() const = 0;

    virtual vector<char> readShader() const;
};