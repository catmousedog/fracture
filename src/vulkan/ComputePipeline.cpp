#include "ComputePipeline.hpp"
#include "vulkan/SlangCompiler.hpp"

////////////////////////////////////////////////////////////

ComputePipeline::ComputePipeline(VulkanCore& vulkanCore, Swappable<VulkanImage>& images, FractalType fractalType)
    : _vulkanCore(vulkanCore),
      _images(images)
{
    createDescriptorPool();
    createDescriptorLayout();
    createDescriptorSet();
    readShader(fractalType);
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorPool()
{
    vk::DescriptorPoolSize poolSize = {
        .type            = vk::DescriptorType::eStorageImage, //
        .descriptorCount = 2                                  //
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 2,
        .poolSizeCount = 1,
        .pPoolSizes    = &poolSize
    };
    _descriptorPool = _vulkanCore.getDevice().createDescriptorPool(poolInfo);
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorLayout()
{
    vk::DescriptorSetLayoutBinding bindings{
        .binding         = 0,
        .descriptorType  = vk::DescriptorType::eStorageImage,
        .descriptorCount = 1,
        .stageFlags      = vk::ShaderStageFlagBits::eCompute
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 1,        //
        .pBindings    = &bindings //
    };
    _descriptorSetLayout = vk::raii::DescriptorSetLayout(_vulkanCore.getDevice(), layoutInfo);
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorSet()
{
    std::vector<vk::DescriptorSetLayout> layouts(2, *_descriptorSetLayout);
    vk::DescriptorSetAllocateInfo        allocInfo{
        .descriptorPool     = _descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()
    };
    _descriptorSet = _vulkanCore.getDevice().allocateDescriptorSets(allocInfo);

    // --- Update Allocated Descriptor --- //
    updateDescriptorSet();
}

////////////////////////////////////////////////////////////

void ComputePipeline::updatePipeline(uint32_t pushConstantSize)
{
    vk::PipelineShaderStageCreateInfo compShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eCompute, .module = _shader, .pName = "compMain"
    };

    // --- Push Constants --- //
    vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eCompute, .offset = 0, .size = pushConstantSize
    };

    /* =================== CREATE PIPELINE ================== */
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount         = 1,
        .pSetLayouts            = &*_descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushConstantRange
    };
    _pipelineLayout = _vulkanCore.getDevice().createPipelineLayout(pipelineLayoutInfo);

    vk::ComputePipelineCreateInfo pipelineInfo{
        .stage  = compShaderStageInfo, //
        .layout = _pipelineLayout      //
    };

    _pipeline = vk::raii::Pipeline(_vulkanCore.getDevice(), nullptr, pipelineInfo);
}

////////////////////////////////////////////////////////////

void ComputePipeline::updateDescriptorSet()
{
    for (uint32_t i = 0; i < _images.size(); ++i)
    {
        vk::DescriptorImageInfo imageInfo{
            .imageView   = _images[i].getComputeImageView(), //
            .imageLayout = vk::ImageLayout::eGeneral         //
        };
        vk::WriteDescriptorSet descriptorWrites{
            .dstSet          = _descriptorSet[i],
            .dstBinding      = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eStorageImage,
            .pImageInfo      = &imageInfo
        };
        _vulkanCore.getDevice().updateDescriptorSets(descriptorWrites, {});
    }
}

////////////////////////////////////////////////////////////

void ComputePipeline::readShader(FractalType fractalType)
{
    std::vector<uint32_t> code = compileComputeShader("compute", toString(fractalType));
    _shader                    = _vulkanCore.createShaderModule(code);
}

////////////////////////////////////////////////////////////

void ComputePipeline::bind(ComputeQueue& computeQueue)
{
    auto& commandBuffer = computeQueue.getCommandBuffer();

    // --- Bind Descriptor Set --- //
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *_pipeline);
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, *_descriptorSet[_images.backIndex()], nullptr
    );
}

////////////////////////////////////////////////////////////