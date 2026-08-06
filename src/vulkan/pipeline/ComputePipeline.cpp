#include "ComputePipeline.hpp"

////////////////////////////////////////////////////////////

ComputePipeline::ComputePipeline(VulkanCore& vulkanCore, VulkanImage& image, const Fractal::Settings& fractalSettings)
    : Pipeline(vulkanCore, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute),
      _image(image),
      _fractalSettings(fractalSettings)
{
    createDescriptorSet();
    createPipeline();
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorSet()
{
    // --- Allocate Descriptor --- //
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool     = _descriptorPool,       //
        .descriptorSetCount = 1,                     //
        .pSetLayouts        = &*_descriptorSetLayout //
    };
    _descriptorSet = std::move(_vulkanCore.getDevice().allocateDescriptorSets(allocInfo).front());

    // --- Update Allocated Descriptor --- //
    updateDescriptorSet();
}

////////////////////////////////////////////////////////////

void ComputePipeline::createPipeline()
{
    // Create Shader
    vk::raii::ShaderModule            shader = _vulkanCore.readShader("compute");
    vk::PipelineShaderStageCreateInfo compShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eCompute, .module = shader, .pName = "compMain"
    };

    // --- Push Constants --- //
    vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eCompute, .offset = 0, .size = sizeof(_fractalSettings)
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
    vk::DescriptorImageInfo imageInfo{
        .imageView   = _image.getComputeImageView(), //
        .imageLayout = vk::ImageLayout::eGeneral     //
    };
    vk::WriteDescriptorSet descriptorWrites{
        .dstSet          = _descriptorSet,
        .dstBinding      = 0,
        .descriptorCount = 1,
        .descriptorType  = vk::DescriptorType::eStorageImage,
        .pImageInfo      = &imageInfo
    };
    _vulkanCore.getDevice().updateDescriptorSets(descriptorWrites, {});
}

////////////////////////////////////////////////////////////

void ComputePipeline::bind(CommandQueue& commandQueue)
{
    auto& commandBuffer = commandQueue.getCommandBuffer();

    // --- Bind Descriptor Set --- //
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *_pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, *_descriptorSet, nullptr);

    // --- Push Constants --- //
    commandBuffer.pushConstants(
        _pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(_fractalSettings), &_fractalSettings
    );
}

////////////////////////////////////////////////////////////