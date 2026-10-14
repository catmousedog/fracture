#include "CPUBackend.hpp"

#include "util/Vulkan.hpp"

////////////////////////////////////////////////////////////

CPUBackend::CPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, VulkanImage& vulkanImage)
    : _vulkanCore(vulkanCore),
      _commandQueue(commandQueue),
      _vulkanImage(vulkanImage)
{
    vk::Extent2D extent = _vulkanImage.getExtent();
    _image              = Image(extent.width, extent.height);
}

////////////////////////////////////////////////////////////

void CPUBackend::recompute()
{

    // --- Create Staging Buffer --- //
    vk::DeviceSize bufferSize                 = _image.size() * sizeof(Image::Pixel);
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // --- Compute Fractal --- //
    fillImage();

    // --- CPUBackend::Image -> Staging Buffer --- //
    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, _image.pixels.data(), bufferSize); // no copy actually needed, do it directly WIP
    stagingBufferMemory.unmapMemory();

    // WIP: KEEP A stagingBuffer for each frame in flight (FrameWorker)
    // Then we can record these commands in the main CommandQueue

    // --- Copy from Staging Buffer to Vulkan Image (with transitions) --- //
    _commandQueue.singleCommand(
        [&](vk::raii::CommandBuffer& commandBuffer)
        {
            // --- Staging Buffer -> Vulkan Image --- //
            util::transitionImageLayout(
                commandBuffer,
                {
                    .srcStageMask  = vk::PipelineStageFlagBits2::eFragmentShader, // wait for frag shader
                    .srcAccessMask = vk::AccessFlagBits2::eShaderRead,            // wait for shader read
                    .dstStageMask  = vk::PipelineStageFlagBits2::eCopy,           // make copy wait
                    .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,         // copy will write
                    .oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal, // might be more efficient to already have it
                                                                          // in transfer dst in GraphicsPipeline?
                    .newLayout = vk::ImageLayout::eTransferDstOptimal,
                    .image     = _vulkanImage.getComputeImage(),
                }
            );

            // --- Staging Buffer -> Vulkan Image --- //
            vk::BufferImageCopy2 region{
                .bufferOffset      = 0,
                .bufferRowLength   = 0, // 0 = tightly packed rows
                .bufferImageHeight = 0,
                .imageSubresource =
                    {.aspectMask     = vk::ImageAspectFlagBits::eColor,
                     .mipLevel       = 0,
                     .baseArrayLayer = 0,
                     .layerCount     = 1},
                .imageOffset = {0, 0, 0},
                .imageExtent = {_image.width, _image.height, 1},
            };
            commandBuffer.copyBufferToImage2({
                .srcBuffer      = *stagingBuffer,
                .dstImage       = *_vulkanImage.getComputeImage(),
                .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                .regionCount    = 1,
                .pRegions       = &region,
            });

            // --- Transition to ShaderReadOnlyOptimal --- //
            util::transitionImageLayout(
                commandBuffer,
                {
                    .srcStageMask  = vk::PipelineStageFlagBits2::eCopy,           // wait for copy
                    .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,         // wait for write
                    .dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader, // make frag shader wait
                    .dstAccessMask = vk::AccessFlagBits2::eShaderRead,            // frag shader will read
                    .oldLayout     = vk::ImageLayout::eTransferDstOptimal,
                    .newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal,
                    .image         = _vulkanImage.getComputeImage(),
                }
            );
        }
    );
}

////////////////////////////////////////////////////////////

void CPUBackend::onResize()
{
    vk::Extent2D extent = _vulkanImage.getExtent();
    _image              = Image(extent.width, extent.height);
}

////////////////////////////////////////////////////////////

void CPUBackend::fillImage()
{
    const float width  = static_cast<float>(_image.width);
    const float height = static_cast<float>(_image.height);
    const float aspect = width / height;

    for (uint32_t y = 0; y < _image.height; y++)
    {
        for (uint32_t x = 0; x < _image.width; x++)
        {
            // Pixel -> [-1, 1], aspect-corrected
            float uvX = (static_cast<float>(x) / width) * 2.0f - 1.0f;
            float uvY = (static_cast<float>(y) / height) * 2.0f - 1.0f;
            uvX *= aspect;

            const float cX = _fractal.offsetX + uvX * _fractal.zoom;
            const float cY = _fractal.offsetY + uvY * _fractal.zoom;

            float zX = 0.0f;
            float zY = 0.0f;
            int   i;
            for (i = 0; i < _fractal.maxIter; i++)
            {
                const float newX = zX * zX - zY * zY + cX;
                const float newY = 2.0f * zX * zY + cY;
                zX               = newX;
                zY               = newY;
                if (zX * zX + zY * zY > 4.0f)
                    break;
            }

            const uint8_t v = static_cast<uint8_t>(255.0f * i / _fractal.maxIter);

            _image[x, y] = {v, v, v, 0xFF};
        }
    }
}
////////////////////////////////////////////////////////////

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
CPUBackend::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
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
