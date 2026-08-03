#include "Pipeline.hpp"

////////////////////////////////////////////////////////////

Pipeline::Pipeline(VulkanCore& vulkanCore, vk::DescriptorType type, vk::ShaderStageFlagBits stage)
    : _vulkanCore(vulkanCore)
{
    createDescriptorPool(type);
    createDescriptorLayout(type, stage);
}

////////////////////////////////////////////////////////////

Pipeline::~Pipeline() = default;

////////////////////////////////////////////////////////////

void Pipeline::createDescriptorPool(vk::DescriptorType type)
{
    vk::DescriptorPoolSize poolSize{.type = type, .descriptorCount = 1};

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = poolSize.descriptorCount,
        .poolSizeCount = 1,
        .pPoolSizes    = &poolSize
    };
    _descriptorPool = _vulkanCore.getDevice().createDescriptorPool(poolInfo);
}

////////////////////////////////////////////////////////////

void Pipeline::createDescriptorLayout(vk::DescriptorType type, vk::ShaderStageFlagBits stage)
{
    // Graphics Descriptor: CombinedImageSampler
    vk::DescriptorSetLayoutBinding bindings{
        .binding         = 0,    //
        .descriptorType  = type, //
        .descriptorCount = 1,    //
        .stageFlags      = stage //
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 1,        //
        .pBindings    = &bindings //
    };
    _descriptorSetLayout = vk::raii::DescriptorSetLayout(_vulkanCore.getDevice(), layoutInfo);
}

////////////////////////////////////////////////////////////