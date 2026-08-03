#include "GraphicsPipeline.hpp"
#include "VertexBuffer.hpp"

////////////////////////////////////////////////////////////

GraphicsPipeline::GraphicsPipeline(VulkanCore& vulkanCore, VulkanImage& image)
    : Pipeline(vulkanCore, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
      _image(image)
{
    createDescriptorSet();
    createPipeline();
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createDescriptorSet()
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

void GraphicsPipeline::updateDescriptorSet()
{
    vk::DescriptorImageInfo imageInfo{
        .sampler     = _image.getSampler(),                    //
        .imageView   = _image.getComputeImageView(),           //
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal //
    };
    vk::WriteDescriptorSet descriptorWrites{
        .dstSet          = _descriptorSet,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo      = &imageInfo
    };
    _vulkanCore.getDevice().updateDescriptorSets(descriptorWrites, {});
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::bind(CommandQueue& commandQueue)
{
    auto& commandBuffer = commandQueue.getCommandBuffer();

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, *_descriptorSet, nullptr);
}

////////////////////////////////////////////////////////////