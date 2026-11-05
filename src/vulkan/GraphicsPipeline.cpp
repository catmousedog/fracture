#include "GraphicsPipeline.hpp"
#include "VertexBuffer.hpp"

////////////////////////////////////////////////////////////

GraphicsPipeline::GraphicsPipeline(VulkanCore& vulkanCore, VulkanImage& vulkanImage)
    : _vulkanCore(vulkanCore),
      _vulkanImage(vulkanImage)
{
    createDescriptorPool();
    createDescriptorLayout();
    createDescriptorSet();
    updateDescriptorSet();
    createPipeline();
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createDescriptorPool()
{
    // --- Pool Sizes --- //
    vk::DescriptorPoolSize poolSize = {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1};

    // --- Create Descriptor Pool --- //
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 1,
        .poolSizeCount = 1,
        .pPoolSizes    = &poolSize
    };
    _descriptorPool = _vulkanCore.getDevice().createDescriptorPool(poolInfo);
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createDescriptorLayout()
{
    // --- Bindings --- //
    vk::DescriptorSetLayoutBinding binding = {
        .binding         = 0,
        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags      = vk::ShaderStageFlagBits::eFragment
    };

    // --- Create Descriptor Set Layout --- //
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 1,       //
        .pBindings    = &binding //
    };
    _descriptorSetLayout = _vulkanCore.getDevice().createDescriptorSetLayout(layoutInfo);
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createDescriptorSet()
{
    // --- Allocate Descriptor Sets --- //
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool     = _descriptorPool,       //
        .descriptorSetCount = 1,                     //
        .pSetLayouts        = &*_descriptorSetLayout //
    };
    _descriptorSet = std::move(_vulkanCore.getDevice().allocateDescriptorSets(allocInfo).front());
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::createPipeline()
{
    // --- Create Shader --- //
    vk::raii::ShaderModule            shader = _vulkanCore.readShaderCode("graphics");
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
        .sampler     = _vulkanImage.getSampler(),              //
        .imageView   = _vulkanImage.getComputeImageView(),     //
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal //
    };
    vk::WriteDescriptorSet descriptorWrite{
        .dstSet          = _descriptorSet,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo      = &imageInfo

    };
    _vulkanCore.getDevice().updateDescriptorSets(descriptorWrite, {});
}

////////////////////////////////////////////////////////////

void GraphicsPipeline::bind(VulkanQueue& graphicsQueue)
{
    auto& commandBuffer = graphicsQueue.getCommandBuffer();

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, *_descriptorSet, nullptr);
}

////////////////////////////////////////////////////////////