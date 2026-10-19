#include "GraphicsPipeline.hpp"
#include "VertexBuffer.hpp"

////////////////////////////////////////////////////////////

GraphicsPipeline::GraphicsPipeline(VulkanCore& vulkanCore, Swappable<VulkanImage>& images, uint32_t framesInFlight)
    : _vulkanCore(vulkanCore),
      _images(images),
      _framesInFlight(framesInFlight)
{
    createDescriptorPool();
    createDescriptorLayout();
    createDescriptorSet();
    createUniformBuffers();
    updateDescriptorSet();
    createPipeline();
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createDescriptorPool()
{
    // --- Pool Sizes --- //
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 2 * _framesInFlight},
        {.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 2 * _framesInFlight},
    }};

    // --- Create Descriptor Pool --- //
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 2 * _framesInFlight,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes    = poolSizes.data()
    };
    _descriptorPool = _vulkanCore.getDevice().createDescriptorPool(poolInfo);
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createDescriptorLayout()
{
    // --- Bindings --- //
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
        {{.binding         = 0,
          .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
          .descriptorCount = 1,
          .stageFlags      = vk::ShaderStageFlagBits::eFragment},
         {.binding         = 1,
          .descriptorType  = vk::DescriptorType::eUniformBuffer,
          .descriptorCount = 1,
          .stageFlags      = vk::ShaderStageFlagBits::eFragment}}
    };

    // --- Create Descriptor Set Layout --- //
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = bindings.size(), //
        .pBindings    = bindings.data()  //
    };
    _descriptorSetLayout = _vulkanCore.getDevice().createDescriptorSetLayout(layoutInfo);
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createDescriptorSet()
{
    // --- Allocate Descriptor Sets --- //
    std::vector<vk::DescriptorSetLayout> layouts(2 * _framesInFlight, *_descriptorSetLayout);
    vk::DescriptorSetAllocateInfo        allocInfo{
        .descriptorPool     = _descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()
    };
    _descriptorSets = _vulkanCore.getDevice().allocateDescriptorSets(allocInfo);
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createPipeline()
{
    // --- Create Shader --- //
    vk::raii::ShaderModule            shader = _vulkanCore.readShader("graphics");
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = shader, .pName = "vertMain"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = shader, .pName = "fragMain"
    };
    vk::PipelineShaderStageCreateInfo shaderStagesInfo[] = {vertShaderStageInfo, fragShaderStageInfo};

    // --- Bind Vertex Buffer --- //
    auto                                   bindingDescription    = VertexBuffer::Vertex::getBindingDescription();
    auto                                   attributeDescriptions = VertexBuffer::Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions    = attributeDescriptions.data()
    };

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{.topology = vk::PrimitiveTopology::eTriangleList};

    // viewport and scissor counts
    vk::PipelineViewportStateCreateInfo viewportInfo{.viewportCount = 1, .scissorCount = 1};

    // rasterization
    vk::PipelineRasterizationStateCreateInfo rasterizationInfo{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f
    };

    // multisampling
    vk::PipelineMultisampleStateCreateInfo multisampleInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False
    };

    // color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo{
        .blendEnable    = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    vk::PipelineColorBlendStateCreateInfo colorBlendInfo{
        .logicOpEnable   = vk::False,
        .logicOp         = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments    = &colorBlendAttachmentInfo
    };

    // viewport and scissors
    vector<vk::DynamicState>           dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()
    };

    /* =================== CREATE PIPELINE ================== */
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1,                     //
        .pSetLayouts    = &*_descriptorSetLayout //
    };
    _pipelineLayout = _vulkanCore.getDevice().createPipelineLayout(pipelineLayoutInfo);

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {.stageCount          = 2,
         .pStages             = shaderStagesInfo,
         .pVertexInputState   = &vertexInputInfo,
         .pInputAssemblyState = &inputAssemblyInfo,
         .pViewportState      = &viewportInfo,
         .pRasterizationState = &rasterizationInfo,
         .pMultisampleState   = &multisampleInfo,
         .pColorBlendState    = &colorBlendInfo,
         .pDynamicState       = &dynamicStateInfo,
         .layout              = _pipelineLayout,
         .renderPass          = nullptr},
        {.colorAttachmentCount = 1, .pColorAttachmentFormats = &_vulkanCore.getSurfaceFormat().format}
    };

    _pipeline = vk::raii::Pipeline(
        _vulkanCore.getDevice(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()
    );
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createUniformBuffers()
{
    _uniformBuffers.resize(_framesInFlight);

    for (uint32_t fr = 0; fr < _framesInFlight; fr++)
    {
        std::tie(_uniformBuffers[fr].buffer, _uniformBuffers[fr].memory) = _vulkanCore.createBuffer(
            sizeof(UniformBufferObject),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        _uniformBuffers[fr].mapped = _uniformBuffers[fr].memory.mapMemory(0, sizeof(UniformBufferObject));
    }
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::updateDescriptorSet()
{
    for (uint32_t im = 0; im < _images.size(); im++)
    {
        for (uint32_t fr = 0; fr < _framesInFlight; fr++)
        {
            uint32_t index = descriptorSetIndex(im, fr);

            vk::DescriptorImageInfo imageInfo{
                .sampler     = _images[im].getSampler(),               //
                .imageView   = _images[im].getComputeImageView(),      //
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal //
            };
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = _uniformBuffers[fr].buffer, .offset = 0, .range = sizeof(UniformBufferObject)
            };

            std::array<vk::WriteDescriptorSet, 2> descriptorWrites{
                {{.dstSet          = _descriptorSets[index],
                  .dstBinding      = 0,
                  .dstArrayElement = 0,
                  .descriptorCount = 1,
                  .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                  .pImageInfo      = &imageInfo},
                 {.dstSet          = _descriptorSets[index],
                  .dstBinding      = 1,
                  .dstArrayElement = 0,
                  .descriptorCount = 1,
                  .descriptorType  = vk::DescriptorType::eUniformBuffer,
                  .pBufferInfo     = &bufferInfo}}
            };
            _vulkanCore.getDevice().updateDescriptorSets(descriptorWrites, {});
        }
    }
}

////////////////////////////////////////////////////////////

// WIP: still needs to accept changing UBO
void GraphicsPipeline::updateUniformBuffer(const GraphicsQueue& graphicsQueue, const UniformBufferObject& ubo)
{
    uint32_t frameIndex = graphicsQueue.getFrameIndex();
    std::memcpy(_uniformBuffers[frameIndex].mapped, &ubo, sizeof(UniformBufferObject));
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::bind(GraphicsQueue& graphicsQueue)
{
    auto& commandBuffer = graphicsQueue.getCommandBuffer();

    uint32_t index = descriptorSetIndex(_images.frontIndex(), graphicsQueue.getFrameIndex());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, *_descriptorSets[index], nullptr
    );
}

////////////////////////////////////////////////////////////