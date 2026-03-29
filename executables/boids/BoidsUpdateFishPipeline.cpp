#include "BoidsUpdateFishPipeline.hpp"
#include <array>

#include "BedrockAssert.hpp"
#include "BedrockPath.hpp"
#include "DescriptorSetSchema.hpp"
#include "ImportShader.hpp"
#include "LogicalDevice.hpp"
#include "RenderBackend.hpp"
#include "vulkan/vulkan_core.h"

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

RT::DescriptorSetGroup BoidsUpdateFishPipeline::CreateFishDescriptorSets(
    MFA::RT::BufferGroup const & stateBuffer,                           // Stores the boids state
    MFA::RT::BufferGroup const & instanceBuffer                         // Stores the final transform used for rendering
) const
{
    MFA_ASSERT(stateBuffer.buffers.size() == instanceBuffer.buffers.size());
    auto const bufferCount = stateBuffer.buffers.size();

    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::GetVkDevice(),
        mDescriptorPool->descriptorPool,
        mFishDescriptorSetLayout->descriptorSetLayout,
        bufferCount
    );

    for (uint32_t bIdx = 0; bIdx < bufferCount; ++bIdx)
    {
        auto const & descriptorSet = descriptorSetGroup.descriptorSets[bIdx];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        auto const bufferInfos = std::to_array<VkDescriptorBufferInfo>({
        {
            .buffer = stateBuffer.buffers[bIdx]->buffer,
            .offset = 0,
            .range = stateBuffer.bufferSize
        },
        {
            .buffer = instanceBuffer.buffers[bIdx]->buffer,
            .offset = 0,
            .range = instanceBuffer.bufferSize
        }});

        DescriptorSetSchema descriptorSetSchema{descriptorSet};

        for (auto const & bufferInfo : bufferInfos)
        {
            descriptorSetSchema.AddStorageBuffer(&bufferInfo);
        }

        descriptorSetSchema.UpdateDescriptorSets();
    }
    return descriptorSetGroup;
}

//----------------------------------------------------------------------------------------------------------------------

void BoidsUpdateFishPipeline::UpdateFishDescriptorSets(
    MFA::RT::DescriptorSetGroup & descriptorSetGroup,
    MFA::RT::BufferGroup const & stateBuffer,                           // Stores the boids state
    MFA::RT::BufferGroup const & instanceBuffer                         // Stores the final transform used for rendering
) const
{
    MFA_ASSERT(stateBuffer.buffers.size() == instanceBuffer.buffers.size());
    MFA_ASSERT(descriptorSetGroup.descriptorSets.size() == stateBuffer.buffers.size());

    auto const bufferCount = stateBuffer.buffers.size();

    for (uint32_t bIdx = 0; bIdx < bufferCount; ++bIdx)
    {
        auto const & descriptorSet = descriptorSetGroup.descriptorSets[bIdx];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        auto const bufferInfos = std::to_array<VkDescriptorBufferInfo>({
        {
            .buffer = stateBuffer.buffers[bIdx]->buffer,
            .offset = 0,
            .range = stateBuffer.bufferSize
        },
        {
            .buffer = instanceBuffer.buffers[bIdx]->buffer,
            .offset = 0,
            .range = instanceBuffer.bufferSize
        }});

        DescriptorSetSchema descriptorSetSchema{descriptorSet};

        for (auto const & bufferInfo : bufferInfos)
        {
            descriptorSetSchema.AddStorageBuffer(&bufferInfo);
        }

        descriptorSetSchema.UpdateDescriptorSets();
    }
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BoidsUpdateFishPipeline::CreateCollisionTrianglesDescriptorSets(
    RT::BufferGroup const & collisionTriangleBuffer
)
{
    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::GetVkDevice(),
        mDescriptorPool->descriptorPool,
        mCollisionTriangleDescriptorSetLayout->descriptorSetLayout,
        collisionTriangleBuffer.buffers.size()
    );

    for (uint32_t bIdx = 0; bIdx < collisionTriangleBuffer.buffers.size(); ++bIdx)
    {
        auto const & descriptorSet = descriptorSetGroup.descriptorSets[bIdx];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        VkDescriptorBufferInfo const bufferInfo{
            .buffer = collisionTriangleBuffer.buffers[bIdx]->buffer,
            .offset = 0,
            .range = collisionTriangleBuffer.bufferSize
        };

        DescriptorSetSchema descriptorSetSchema{descriptorSet};
        descriptorSetSchema.AddStorageBuffer(&bufferInfo);
        descriptorSetSchema.UpdateDescriptorSets();
    }

    return descriptorSetGroup;
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BoidsUpdateFishPipeline::CreateSimulationConstantsDescriptorSets(
    RT::BufferGroup const & simulationConstantsBuffer
)
{
    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::GetVkDevice(),
        mDescriptorPool->descriptorPool,
        mSimulationConstantsDescriptorSetLayout->descriptorSetLayout,
        simulationConstantsBuffer.buffers.size()
    );

    for (uint32_t bIdx = 0; bIdx < simulationConstantsBuffer.buffers.size(); ++bIdx)
    {
        auto const & descriptorSet = descriptorSetGroup.descriptorSets[bIdx];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        VkDescriptorBufferInfo const bufferInfo{
            .buffer = simulationConstantsBuffer.buffers[bIdx]->buffer,
            .offset = 0,
            .range = simulationConstantsBuffer.bufferSize
        };

        DescriptorSetSchema descriptorSetSchema{descriptorSet};
        descriptorSetSchema.AddUniformBuffer(&bufferInfo);
        descriptorSetSchema.UpdateDescriptorSets();
    }

    return descriptorSetGroup;
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
        auto bindings = std::to_array<VkDescriptorSetLayoutBinding>({
            VkDescriptorSetLayoutBinding {                      // Fish states
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            },
            {                                                   // Fish instances
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            }
        });

        mFishDescriptorSetLayout = RB::CreateDescriptorSetLayout(
            LogicalDevice::GetVkDevice(),
            bindings.size(),
            bindings.data()
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
