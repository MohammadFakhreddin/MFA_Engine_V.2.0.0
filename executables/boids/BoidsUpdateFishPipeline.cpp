#include "BoidsUpdateFishPipeline.hpp"

#include "BedrockAssert.hpp"
#include "BedrockPath.hpp"
#include "DescriptorSetSchema.hpp"
#include "ImportShader.hpp"
#include "LogicalDevice.hpp"
#include "RenderBackend.hpp"

using namespace MFA;

//----------------------------------------------------------------------------------------------------------------------

BoidsUpdateFishPipeline::BoidsUpdateFishPipeline()
    : BoidsUpdateFishPipeline(Params{})
{
}

//----------------------------------------------------------------------------------------------------------------------

BoidsUpdateFishPipeline::BoidsUpdateFishPipeline(
    Params params
)
    : mParams(params)
{
    mDescriptorPool = RB::CreateDescriptorPool(
        LogicalDevice::GetVkDevice(),
        mParams.maxSets * 3u
    );

    CreateDescriptorSetLayouts();
    CreatePipeline();
}

//----------------------------------------------------------------------------------------------------------------------

BoidsUpdateFishPipeline::~BoidsUpdateFishPipeline()
{
    mPipeline = nullptr;
    mSimulationConstantsDescriptorSetLayout = nullptr;
    mCollisionTriangleDescriptorSetLayout = nullptr;
    mFishDescriptorSetLayout = nullptr;
    mDescriptorPool = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

bool BoidsUpdateFishPipeline::IsBinded(RT::CommandRecordState const & recordState) const
{
    return recordState.pipeline == mPipeline.get();
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::BindPipeline(RT::CommandRecordState & recordState) const
{
    if (IsBinded(recordState))
    {
        return;
    }

    RB::BindPipeline(recordState, *mPipeline);
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::BindFishes(
    MFA::RT::CommandRecordState const & recordState,
    MFA::RT::DescriptorSetGroup const & fishesDescriptorSet
) const
{
    RB::AutoBindDescriptorSet(recordState, (RB::UpdateFrequency)0, fishesDescriptorSet);
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::BindCollisionTriangles(
    MFA::RT::CommandRecordState const & recordState,
    MFA::RT::DescriptorSetGroup const & collisionTrianglesDescriptorSet
) const
{
    RB::AutoBindDescriptorSet(recordState, (RB::UpdateFrequency)1, collisionTrianglesDescriptorSet);
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::BindSimulationConstants(
    MFA::RT::CommandRecordState const & recordState,
    MFA::RT::DescriptorSetGroup const & simulationConstantsDescriptorSet
) const
{
    RB::AutoBindDescriptorSet(recordState, (RB::UpdateFrequency)2, simulationConstantsDescriptorSet);
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::SetPushConstants(
    RT::CommandRecordState & recordState,
    PushConstants const & pushConstants
) const
{
    RB::PushConstants(
        recordState,
        mPipeline->pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        Alias{pushConstants}
    );
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BoidsUpdateFishPipeline::CreateFishesDescriptorSets(
    RT::BufferGroup const & fishBuffer
) const
{
    return CreateStorageBufferDescriptorSets(fishBuffer, *mFishDescriptorSetLayout);
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BoidsUpdateFishPipeline::CreateCollisionTrianglesDescriptorSets(
    RT::BufferGroup const & collisionTriangleBuffer
) const
{
    return CreateStorageBufferDescriptorSets(
        collisionTriangleBuffer,
        *mCollisionTriangleDescriptorSetLayout
    );
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BoidsUpdateFishPipeline::CreateSimulationConstantsDescriptorSets(
    RT::BufferGroup const & simulationConstantsBuffer
) const
{
    return CreateUniformBufferDescriptorSets(
        simulationConstantsBuffer,
        *mSimulationConstantsDescriptorSetLayout
    );
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::Reload()
{
    CreatePipeline();
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::CreateDescriptorSetLayouts()
{
    {
        VkDescriptorSetLayoutBinding binding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        };

        mFishDescriptorSetLayout = RB::CreateDescriptorSetLayout(
            LogicalDevice::GetVkDevice(),
            1,
            &binding
        );
    }

    {
        VkDescriptorSetLayoutBinding binding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        };

        mCollisionTriangleDescriptorSetLayout = RB::CreateDescriptorSetLayout(
            LogicalDevice::GetVkDevice(),
            1,
            &binding
        );
    }

    {
        VkDescriptorSetLayoutBinding binding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        };

        mSimulationConstantsDescriptorSetLayout = RB::CreateDescriptorSetLayout(
            LogicalDevice::GetVkDevice(),
            1,
            &binding
        );
    }
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::CreatePipeline()
{
    bool success = Importer::CompileShaderToSPV(
        Path::Get("shaders/boids/boids_update_fish.comp"),
        Path::Get("shaders/boids/boids_update_fish.comp.spv"),
        "comp"
    );
    MFA_ASSERT(success == true);

    auto cpuComputeShader = Importer::ShaderFromSPV(
        Path::Get("shaders/boids/boids_update_fish.comp.spv"),
        VK_SHADER_STAGE_COMPUTE_BIT,
        "main"
    );
    auto gpuComputeShader = RB::CreateShader(
        LogicalDevice::GetVkDevice(),
        cpuComputeShader
    );

    VkPushConstantRange const pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushConstants)
    };

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
        mFishDescriptorSetLayout->descriptorSetLayout,
        mCollisionTriangleDescriptorSetLayout->descriptorSetLayout,
        mSimulationConstantsDescriptorSetLayout->descriptorSetLayout
    };

    auto const pipelineLayout = RB::CreatePipelineLayout(
        LogicalDevice::GetVkDevice(),
        static_cast<uint32_t>(descriptorSetLayouts.size()),
        descriptorSetLayouts.data(),
        1,
        &pushConstantRange
    );

    mPipeline = RB::CreateComputePipeline(
        LogicalDevice::GetVkDevice(),
        *gpuComputeShader,
        pipelineLayout
    );
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BoidsUpdateFishPipeline::CreateStorageBufferDescriptorSets(
    RT::BufferGroup const & bufferGroup,
    RT::DescriptorSetLayoutGroup const & descriptorSetLayout
) const
{
    auto const maxFramesPerFlight = LogicalDevice::GetMaxFramePerFlight();
    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::GetVkDevice(),
        mDescriptorPool->descriptorPool,
        descriptorSetLayout.descriptorSetLayout,
        maxFramesPerFlight
    );

    for (uint32_t frameIndex = 0; frameIndex < maxFramesPerFlight; ++frameIndex)
    {
        auto const & descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        VkDescriptorBufferInfo const bufferInfo{
            .buffer = bufferGroup.buffers[frameIndex]->buffer,
            .offset = 0,
            .range = bufferGroup.bufferSize
        };

        DescriptorSetSchema descriptorSetSchema{descriptorSet};
        descriptorSetSchema.AddStorageBuffer(&bufferInfo);
        descriptorSetSchema.UpdateDescriptorSets();
    }

    return descriptorSetGroup;
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BoidsUpdateFishPipeline::CreateUniformBufferDescriptorSets(
    RT::BufferGroup const & bufferGroup,
    RT::DescriptorSetLayoutGroup const & descriptorSetLayout
) const
{
    auto const maxFramesPerFlight = LogicalDevice::GetMaxFramePerFlight();
    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::GetVkDevice(),
        mDescriptorPool->descriptorPool,
        descriptorSetLayout.descriptorSetLayout,
        maxFramesPerFlight
    );

    for (uint32_t frameIndex = 0; frameIndex < maxFramesPerFlight; ++frameIndex)
    {
        auto const & descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        VkDescriptorBufferInfo const bufferInfo{
            .buffer = bufferGroup.buffers[frameIndex]->buffer,
            .offset = 0,
            .range = bufferGroup.bufferSize
        };

        DescriptorSetSchema descriptorSetSchema{descriptorSet};
        descriptorSetSchema.AddUniformBuffer(&bufferInfo);
        descriptorSetSchema.UpdateDescriptorSets();
    }

    return descriptorSetGroup;
}
