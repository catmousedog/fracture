#pragma once

#include "CommandQueue.hpp"
#include "VulkanCore.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "util/Basics.hpp"

class VertexBuffer
{
    /* ==================== NESTED CLASS ==================== */
  public:
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec2 texCoord;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            return {.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex};
        }

        static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
        {
            return {
                {{.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)},
                 {.location = 1,
                  .binding  = 0,
                  .format   = vk::Format::eR32G32Sfloat,
                  .offset   = offsetof(Vertex, texCoord)}}
            };
        }
    };

    /* ======================== SETUP ======================= */

    VertexBuffer(VulkanCore& vulkanCore, CommandQueue& commandQueue);

  private:
    void createVertexBuffer();
    void createIndexBuffer();

    /* ==================== CAPABILITIES ==================== */
  public:
    void bind(CommandQueue& commandQueue);

    void draw(CommandQueue& commandQueue);

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
    createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore&   _vulkanCore;
    CommandQueue& _commandQueue;

    // --- Fullscreen Quad --- //
    // Vertex Buffer
    std::vector<Vertex>    _vertices           = {};
    vk::raii::Buffer       _vertexBuffer       = nullptr;
    vk::raii::DeviceMemory _vertexBufferMemory = nullptr;
    // Index Buffer
    vector<uint16_t>       _indices           = {};
    vk::raii::Buffer       _indexBuffer       = nullptr;
    vk::raii::DeviceMemory _indexBufferMemory = nullptr;
};