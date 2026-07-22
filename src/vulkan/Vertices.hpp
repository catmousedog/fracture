#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec2 pos;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return {.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex};
    }

    static vk::VertexInputAttributeDescription getAttributeDescriptions()
    {
        return {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)};
    }
};

struct FractalPushConstants
{
    float offsetX, offsetY;
    float zoom;
    int   maxIter;
};