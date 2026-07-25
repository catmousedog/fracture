#include "Fractal.hpp"

#include <fstream>

#include "util/Log.hpp"

////////////////////////////////////////////////////////////

Fractal::Fractal() { }

////////////////////////////////////////////////////////////

Fractal::~Fractal() { }

////////////////////////////////////////////////////////////

vector<char> Fractal::readShader() const
{
    string shaderPath = SHADER_DIR + shaderFileName() + ".spv";

    // seek end of file
    std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        FATAL("failed to open shader at {}!", shaderPath);

    vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();

    return buffer;
}

////////////////////////////////////////////////////////////