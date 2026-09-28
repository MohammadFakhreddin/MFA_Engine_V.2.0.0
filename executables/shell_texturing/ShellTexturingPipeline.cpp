#include "ShellTexturingPipeline.hpp"

#include "BedrockLog.hpp"
#include "BedrockPath.hpp"
#include "DescriptorSetSchema.hpp"
#include "ImportShader.hpp"
#include "LogicalDevice.hpp"
#include "RenderBackend.hpp"

#include <array>
#include <vector>

using namespace MFA;

namespace
{
    enum DescriptorSetIndex : uint32_t
    {
        CameraSet = 0,
        ShellSet = 1,
        ModelSet = 2,
        MaterialSet = 3,
        AlbedoTextureSet = 4,
        LightSet = 5,
    };
}

//======================================================================================================================

ShellTexturingPipeline::ShellTexturingPipeline(VkRenderPass const renderPass)
    : _renderPass(renderPass)
{
    constexpr uint32_t descriptorCapacity = 64;
    _descriptorPool = RB::CreateDescriptorPool(LogicalDevice::GetVkDevice(), descriptorCapacity);
    CreateDescriptorSetLayouts();
    CreatePipeline();
}

//======================================================================================================================

ShellTexturingPipeline::~ShellTexturingPipeline()
{
    _pipeline = nullptr;
    _descriptorSetLayouts = {};
    _descriptorPool = nullptr;
}

//======================================================================================================================

void ShellTexturingPipeline::BindPipeline(RT::CommandRecordState &recordState) const
{
    if (recordState.pipeline != _pipeline.get())
    {
        RB::BindPipeline(recordState, *_pipeline);
    }
}

//======================================================================================================================

void ShellTexturingPipeline::BindDescriptorSets(
    RT::CommandRecordState const &recordState,
    RT::DescriptorSetGroup const &camera,
    RT::DescriptorSetGroup const &shell,
    RT::DescriptorSetGroup const &model,
    RT::DescriptorSetGroup const &material,
    RT::DescriptorSetGroup const &albedoTexture,
    RT::DescriptorSetGroup const &light
) const
{
    RB::AutoBindDescriptorSet(recordState, static_cast<RB::UpdateFrequency>(CameraSet), camera);
    RB::AutoBindDescriptorSet(recordState, static_cast<RB::UpdateFrequency>(ShellSet), shell);
    RB::AutoBindDescriptorSet(recordState, static_cast<RB::UpdateFrequency>(ModelSet), model);
    RB::AutoBindDescriptorSet(recordState, static_cast<RB::UpdateFrequency>(MaterialSet), material);
    RB::AutoBindDescriptorSet(recordState, static_cast<RB::UpdateFrequency>(AlbedoTextureSet), albedoTexture);
    RB::AutoBindDescriptorSet(recordState, static_cast<RB::UpdateFrequency>(LightSet), light);
}

//======================================================================================================================

RT::DescriptorSetGroup ShellTexturingPipeline::CreateCameraDescriptorSets(RT::BufferGroup const &buffer) const
{
    return CreateUniformDescriptorSets(buffer, *_descriptorSetLayouts[CameraSet]);
}

//======================================================================================================================

RT::DescriptorSetGroup ShellTexturingPipeline::CreateShellDescriptorSets(RT::BufferGroup const &buffer) const
{
    return CreateUniformDescriptorSets(buffer, *_descriptorSetLayouts[ShellSet]);
}

//======================================================================================================================

RT::DescriptorSetGroup ShellTexturingPipeline::CreateModelDescriptorSets(RT::BufferGroup const &buffer) const
{
    return CreateUniformDescriptorSets(buffer, *_descriptorSetLayouts[ModelSet]);
}

//======================================================================================================================

RT::DescriptorSetGroup ShellTexturingPipeline::CreateMaterialDescriptorSets(RT::BufferGroup const &buffer) const
{
    return CreateUniformDescriptorSets(buffer, *_descriptorSetLayouts[MaterialSet]);
}

//======================================================================================================================

RT::DescriptorSetGroup ShellTexturingPipeline::CreateAlbedoTextureDescriptorSet(
    RT::SamplerGroup const &sampler,
    RT::GpuTexture const &texture
) const
{
    MFA_ASSERT(texture.imageView != nullptr);

    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::GetVkDevice(),
        _descriptorPool->descriptorPool,
        _descriptorSetLayouts[AlbedoTextureSet]->descriptorSetLayout,
        1
    );

    VkDescriptorImageInfo const imageInfo{
        .sampler = sampler.sampler,
        .imageView = texture.imageView->imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    DescriptorSetSchema schema{descriptorSetGroup.descriptorSets.front()};
    schema.AddCombinedImageSampler(&imageInfo);
    schema.UpdateDescriptorSets();

    return descriptorSetGroup;
}

//======================================================================================================================

RT::DescriptorSetGroup ShellTexturingPipeline::CreateLightDescriptorSets(RT::BufferGroup const &buffer) const
{
    return CreateUniformDescriptorSets(buffer, *_descriptorSetLayouts[LightSet]);
}

//======================================================================================================================

RT::DescriptorSetGroup ShellTexturingPipeline::CreateUniformDescriptorSets(
    RT::BufferGroup const &buffer,
    RT::DescriptorSetLayoutGroup const &layout
) const
{
    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::GetVkDevice(),
        _descriptorPool->descriptorPool,
        layout.descriptorSetLayout,
        static_cast<uint32_t>(buffer.buffers.size())
    );

    for (size_t bufferIndex = 0; bufferIndex < buffer.buffers.size(); ++bufferIndex)
    {
        VkDescriptorBufferInfo const bufferInfo{
            .buffer = buffer.buffers[bufferIndex]->buffer,
            .offset = 0,
            .range = buffer.bufferSize,
        };
        DescriptorSetSchema schema{descriptorSetGroup.descriptorSets[bufferIndex]};
        schema.AddUniformBuffer(&bufferInfo);
        schema.UpdateDescriptorSets();
    }

    return descriptorSetGroup;
}

//======================================================================================================================

void ShellTexturingPipeline::Reload()
{
    MFA_LOG_DEBUG("Reloading shell texturing pipeline");
    LogicalDevice::DeviceWaitIdle();
    CreatePipeline();
}

//======================================================================================================================

void ShellTexturingPipeline::CreateDescriptorSetLayouts()
{
    auto const createUniformLayout = [](VkShaderStageFlags const stageFlags)
    {
        VkDescriptorSetLayoutBinding binding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = stageFlags,
        };
        return RB::CreateDescriptorSetLayout(LogicalDevice::GetVkDevice(), 1, &binding);
    };

    _descriptorSetLayouts[CameraSet] = createUniformLayout(VK_SHADER_STAGE_VERTEX_BIT);
    _descriptorSetLayouts[ShellSet] = createUniformLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    _descriptorSetLayouts[ModelSet] = createUniformLayout(VK_SHADER_STAGE_VERTEX_BIT);
    _descriptorSetLayouts[MaterialSet] = createUniformLayout(VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayoutBinding textureBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    _descriptorSetLayouts[AlbedoTextureSet] =
        RB::CreateDescriptorSetLayout(LogicalDevice::GetVkDevice(), 1, &textureBinding);

    _descriptorSetLayouts[LightSet] = createUniformLayout(VK_SHADER_STAGE_FRAGMENT_BIT);
}

//======================================================================================================================

void ShellTexturingPipeline::CreatePipeline()
{
    auto const compileShader = [](char const *sourcePath, char const *binaryPath, char const *stage)
    {
        auto const compiled = Importer::CompileShaderToSPV(Path::Get(sourcePath), Path::Get(binaryPath), stage);
        MFA_ASSERT(compiled == true);
    };

    compileShader(
        "shaders/shell_texturing/shell_texturing.vert",
        "shaders/shell_texturing/shell_texturing.vert.spv",
        "vert"
    );
    compileShader(
        "shaders/shell_texturing/shell_texturing.frag",
        "shaders/shell_texturing/shell_texturing.frag.spv",
        "frag"
    );

    auto const cpuVertexShader = Importer::ShaderFromSPV(
        Path::Get("shaders/shell_texturing/shell_texturing.vert.spv"),
        VK_SHADER_STAGE_VERTEX_BIT,
        "main"
    );
    auto const cpuFragmentShader = Importer::ShaderFromSPV(
        Path::Get("shaders/shell_texturing/shell_texturing.frag.spv"),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        "main"
    );
    auto const gpuVertexShader = RB::CreateShader(LogicalDevice::GetVkDevice(), cpuVertexShader);
    auto const gpuFragmentShader = RB::CreateShader(LogicalDevice::GetVkDevice(), cpuFragmentShader);
    std::array<RT::GpuShader const *, 2> shaders{gpuVertexShader.get(), gpuFragmentShader.get()};

    std::array<VkVertexInputBindingDescription, 1> const bindingDescriptions{
        VkVertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{
        VkVertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, position),
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, normal),
        },
        VkVertexInputAttributeDescription{
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, uv),
        },
        VkVertexInputAttributeDescription{
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, center),
        },
    };

    std::array<VkDynamicState, 2> const dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    RB::CreateGraphicPipelineOptions pipelineOptions{};
    pipelineOptions.useStaticViewportAndScissor = false;
    pipelineOptions.rasterizationSamples = LogicalDevice::GetMaxSampleCount();
    // pipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineOptions.cullMode = VK_CULL_MODE_NONE;
    pipelineOptions.depthStencil.depthWriteEnable = VK_FALSE;
    pipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineOptions.dynamicStateCreateInfo = &dynamicStateInfo;

    std::array<VkDescriptorSetLayout, 6> descriptorSetLayouts{};
    for (size_t index = 0; index < descriptorSetLayouts.size(); ++index)
    {
        descriptorSetLayouts[index] = _descriptorSetLayouts[index]->descriptorSetLayout;
    }
    auto const pipelineLayout = RB::CreatePipelineLayout(
        LogicalDevice::GetVkDevice(),
        static_cast<uint32_t>(descriptorSetLayouts.size()),
        descriptorSetLayouts.data(),
        0,
        nullptr
    );

    _pipeline = RB::CreateGraphicPipeline(
        LogicalDevice::GetVkDevice(),
        static_cast<uint8_t>(shaders.size()),
        shaders.data(),
        static_cast<uint32_t>(bindingDescriptions.size()),
        bindingDescriptions.data(),
        static_cast<uint8_t>(attributeDescriptions.size()),
        attributeDescriptions.data(),
        LogicalDevice::GetSurfaceCapabilities().currentExtent,
        _renderPass,
        pipelineLayout,
        pipelineOptions
    );
}
