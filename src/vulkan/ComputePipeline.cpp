#include "ComputePipeline.hpp"
#include "vulkan/SlangCompiler.hpp"

////////////////////////////////////////////////////////////

ComputePipeline::ComputePipeline(VulkanCore& vulkanCore, VulkanImage& vulkanImage, const Backend::Settings& settings)
    : _vulkanCore(vulkanCore),
      _vulkanImage(vulkanImage)
{
    createDescriptorPool();
    createDescriptorLayout();
    createDescriptorSet();
    createUniformBuffer();
    updateDescriptorSet();
    readShader(settings);
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorPool()
{
    // --- Pool Sizes --- //
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {.type = vk::DescriptorType::eStorageImage, .descriptorCount = 1},
        {.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1},
    }};

    // --- Create Descriptor Pool --- //
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 1,
        .poolSizeCount = 2,
        .pPoolSizes    = poolSizes.data()
    };
    _descriptorPool = _vulkanCore.getDevice().createDescriptorPool(poolInfo);
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorLayout()
{
    // --- Bindings --- //
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
        {{.binding         = 0,
          .descriptorType  = vk::DescriptorType::eStorageImage,
          .descriptorCount = 1,
          .stageFlags      = vk::ShaderStageFlagBits::eCompute},
         {.binding         = 1,
          .descriptorType  = vk::DescriptorType::eUniformBuffer,
          .descriptorCount = 1,
          .stageFlags      = vk::ShaderStageFlagBits::eCompute}}
    };

    // --- Create Descriptor Set Layout --- //
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = bindings.size(), //
        .pBindings    = bindings.data()  //
    };
    _descriptorSetLayout = _vulkanCore.getDevice().createDescriptorSetLayout(layoutInfo);
}

////////////////////////////////////////////////////////////

void ComputePipeline::createDescriptorSet()
{
    // --- Allocate Descriptor Set --- //
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool     = _descriptorPool,       //
        .descriptorSetCount = 1,                     //
        .pSetLayouts        = &*_descriptorSetLayout //
    };
    _descriptorSet = std::move(_vulkanCore.getDevice().allocateDescriptorSets(allocInfo).front());
}

////////////////////////////////////////////////////////////

void ComputePipeline::updatePipeline()
{
    vk::PipelineShaderStageCreateInfo compShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eCompute, .module = _shader, .pName = "compMain"
    };

    // --- Push Constants --- //
    vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eCompute, .offset = 0, .size = sizeof(Constants)
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

void ComputePipeline::createUniformBuffer()
{
    auto [_buffer, _memory] = _vulkanCore.createBuffer(
        sizeof(UniformBufferObject),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    _mapped = _memory.mapMemory(0, sizeof(UniformBufferObject));
}

////////////////////////////////////////////////////////////

void ComputePipeline::updateDescriptorSet()
{
    // --- Image --- //
    vk::DescriptorImageInfo imageInfo{
        .imageView   = _vulkanImage.getComputeImageView(), //
        .imageLayout = vk::ImageLayout::eGeneral           //
    };
    // --- UBO --- //
    vk::DescriptorBufferInfo bufferInfo{.buffer = _buffer, .offset = 0, .range = sizeof(UniformBufferObject)};

    std::array<vk::WriteDescriptorSet, 2> descriptorWrites{
        {{.dstSet          = _descriptorSet,
          .dstBinding      = 0,
          .descriptorCount = 1,
          .descriptorType  = vk::DescriptorType::eStorageImage,
          .pImageInfo      = &imageInfo},
         {.dstSet          = _descriptorSet,
          .dstBinding      = 1,
          .descriptorCount = 1,
          .descriptorType  = vk::DescriptorType::eUniformBuffer,
          .pBufferInfo     = &bufferInfo}}
    };
    _vulkanCore.getDevice().updateDescriptorSets(descriptorWrites, {});
}

////////////////////////////////////////////////////////////

void ComputePipeline::updateUniformBuffer(const UniformBufferObject& ubo)
{
    std::memcpy(_mapped, &ubo, sizeof(UniformBufferObject));
}

////////////////////////////////////////////////////////////

void ComputePipeline::readShader(const Backend::Settings& settings)
{
    string                fractalName  = settings.fractal.toString();
    string                functionName = settings.function.toString();
    string                filterName   = settings.filter.toString();
    std::vector<uint32_t> code = compileComputeShader("compute", real_name(), fractalName, functionName, filterName);
    _shader                    = _vulkanCore.createShaderModule(code);
}

////////////////////////////////////////////////////////////

void ComputePipeline::bind(VulkanQueue& computeQueue)
{
    auto& commandBuffer = computeQueue.getCommandBuffer();

    // --- Bind Descriptor Set --- //
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *_pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, *_descriptorSet, nullptr);
}

////////////////////////////////////////////////////////////