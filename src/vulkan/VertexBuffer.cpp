#include "VertexBuffer.hpp"

////////////////////////////////////////////////////////////

VertexBuffer::VertexBuffer(VulkanCore& vulkanCore, CommandQueue& commandQueue)
    : _vulkanCore(vulkanCore),
      _commandQueue(commandQueue),

      // --- Fullscreen Quad --- //
      _vertices(
          {{{-1.f, -1.f}, {0.0f, 1.0f}},
           {{1.f, -1.f}, {1.0f, 1.0f}},
           {{1.f, 1.f}, {1.0f, 0.0f}},
           {{-1.f, 1.f}, {0.0f, 0.0f}}}
      ),
      _indices({0, 1, 2, 2, 3, 0})
{
    createVertexBuffer();
    createIndexBuffer();
}

////////////////////////////////////////////////////////////

void VertexBuffer::createVertexBuffer()
{
    // --- Create Staging Buffer --- //
    vk::DeviceSize bufferSize                 = sizeof(_vertices[0]) * _vertices.size();
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // --- Copy to Staging Buffer --- //
    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, _vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    // --- Create Vertex Buffer --- //
    std::tie(_vertexBuffer, _vertexBufferMemory) = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    // --- Transfer from Staging Buffer --- //
    _commandQueue.singleCommand(
        [&](vk::raii::CommandBuffer& commandBuffer)
        { commandBuffer.copyBuffer(*stagingBuffer, *_vertexBuffer, vk::BufferCopy{.size = bufferSize}); }
    );
}

////////////////////////////////////////////////////////////

void VertexBuffer::createIndexBuffer()
{
    // --- Create Staging Buffer --- //
    vk::DeviceSize bufferSize                 = sizeof(_indices[0]) * _indices.size();
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // --- Copy to Staging Buffer --- //
    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, _indices.data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    // --- Create Index Buffer --- //
    std::tie(_indexBuffer, _indexBufferMemory) = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    // --- Transfer from Staging Buffer --- //
    _commandQueue.singleCommand(
        [&](vk::raii::CommandBuffer& commandBuffer)
        { commandBuffer.copyBuffer(*stagingBuffer, *_indexBuffer, vk::BufferCopy{.size = bufferSize}); }
    );
}

////////////////////////////////////////////////////////////

void VertexBuffer::draw(CommandQueue& commandQueue)
{
    auto& commandBuffer = commandQueue.getCommandBuffer();

    commandBuffer.bindVertexBuffers(0, *_vertexBuffer, {0});
    commandBuffer.bindIndexBuffer(*_indexBuffer, 0, vk::IndexType::eUint16);
    commandBuffer.drawIndexed(static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
}

////////////////////////////////////////////////////////////

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
VertexBuffer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    auto& device = _vulkanCore.getDevice();

    vk::BufferCreateInfo   bufferInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
    vk::raii::Buffer       buffer          = vk::raii::Buffer(device, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = _vulkanCore.findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
    return {std::move(buffer), std::move(bufferMemory)};
}

////////////////////////////////////////////////////////////