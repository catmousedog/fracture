#pragma once

#include "util/Basics.hpp"
#include <cstdint>

// realType: "float" or "double", passed to the shader as the REAL macro.
vector<uint32_t> compileComputeShader(
    string shaderName, string precisionType, string fractalName, string functionName, string filterName
);
