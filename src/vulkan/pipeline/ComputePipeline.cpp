#include "ComputePipeline.hpp"

////////////////////////////////////////////////////////////

ComputePipeline::ComputePipeline(VulkanCore& vulkanCore, VulkanImage& image, const Fractal::Settings& fractalSettings)
    : Pipeline(vulkanCore),
      _image(image),
      _fractalSettings(fractalSettings)
{
    createDescriptorPool();
    createDescriptorLayout();
    createDescriptorSet();
    createPipeline();
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eStorageImage, 1}, //
        {vk::DescriptorType::eStorageBuffer, 1} //
    }};

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 2,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes    = poolSizes.data()
    };
    _descriptorPool = _vulkanCore.getDevice().createDescriptorPool(poolInfo);
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
        {{.binding         = 0,
          .descriptorType  = vk::DescriptorType::eStorageImage,
          .descriptorCount = 1,
          .stageFlags      = vk::ShaderStageFlagBits::eCompute},
         {.binding         = 1,
          .descriptorType  = vk::DescriptorType::eStorageBuffer,
          .descriptorCount = 1,
          .stageFlags      = vk::ShaderStageFlagBits::eCompute}}
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = bindings.size(), //
        .pBindings    = bindings.data()  //
    };
    _descriptorSetLayout = vk::raii::DescriptorSetLayout(_vulkanCore.getDevice(), layoutInfo);
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