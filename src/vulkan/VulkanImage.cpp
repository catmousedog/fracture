
#include "VulkanImage.hpp"

#include "util/Vulkan.hpp"

////////////////////////////////////////////////////////////

VulkanImage::VulkanImage(VulkanCore& vulkanCore, CommandQueue& commandQueue)
    : _vulkanCore(vulkanCore),
      _commandQueue(commandQueue)
{
    createComputeImage();
    createSampler();
}

////////////////////////////////////////////////////////////

void VulkanImage::createComputeImage()
{
    auto  extent = _vulkanCore.getSwapChainExtent();
    auto& device = _vulkanCore.getDevice();

    constexpr vk::Format format = vk::Format::eR8G8B8A8Unorm;

    /* ==================== CREATE IMAGE ==================== */
    vk::ImageCreateInfo imageInfo{
        .imageType     = vk::ImageType::e2D,
        .format        = format,
        .extent        = {extent.width, extent.height, 1},
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = vk::SampleCountFlagBits::e1,
        .tiling        = vk::ImageTiling::eOptimal,
        .usage         = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
        .sharingMode   = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };
    _computeImage = vk::raii::Image(device, imageInfo);

    // --- Create/Bind Image Memory --- //
    vk::MemoryRequirements memRequirements = _computeImage.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            _vulkanCore.findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };
    _computeImageMemory = vk::raii::DeviceMemory(device, allocInfo);
    _computeImage.bindMemory(*_computeImageMemory, 0);

    /* ================== CREATE IMAGE VIEW ================= */
    vk::ImageViewCreateInfo viewInfo{
        .image            = _computeImage,
        .viewType         = vk::ImageViewType::e2D,
        .format           = format,
        .subresourceRange = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    _computeImageView = vk::raii::ImageView(device, viewInfo);

    /* ================== TRANSITION IMAGE ================== */
    _commandQueue.singleCommand(
        [this](vk::raii::CommandBuffer& commandBuffer)
        {
            util::transitionImageLayout(
                commandBuffer,
                {
                    .srcStageMask  = vk::PipelineStageFlagBits2::eTopOfPipe,
                    .srcAccessMask = {},
                    .dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
                    .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                    .oldLayout     = vk::ImageLayout::eUndefined,
                    .newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal,
                    .image         = _computeImage,
                }
            );
        }
    );
}

////////////////////////////////////////////////////////////

void VulkanImage::createSampler()
{
    // Graphics Image Sampler
    auto& device         = _vulkanCore.getDevice();
    auto& physicalDevice = _vulkanCore.getPhysicalDevice();

    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo        samplerInfo{
        .magFilter        = vk::Filter::eLinear,
        .minFilter        = vk::Filter::eLinear,
        .mipmapMode       = vk::SamplerMipmapMode::eLinear,
        .addressModeU     = vk::SamplerAddressMode::eRepeat,
        .addressModeV     = vk::SamplerAddressMode::eRepeat,
        .addressModeW     = vk::SamplerAddressMode::eRepeat,
        .mipLodBias       = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
        .compareEnable    = vk::False,
        .compareOp        = vk::CompareOp::eAlways
    };
    _sampler = device.createSampler(samplerInfo);
}

////////////////////////////////////////////////////////////

void VulkanImage::recreateComputeImage()
{
    _computeImage.clear();
    _computeImageMemory.clear();
    _computeImageView.clear();

    createComputeImage();
}

////////////////////////////////////////////////////////////