#pragma once

#include "pipeline/IShadingPipeline.hpp"
#include "RenderTypes.hpp"

#include <cstdint>

class BoidsUpdateFishPipeline : public MFA::IShadingPipeline
{
public:

    struct PushConstants
    {
        float dt{};
        float fixedDt{};
        uint32_t stateMask{};
    };

    struct Params
    {
        uint32_t maxSets = 100;
    };

    BoidsUpdateFishPipeline();

    explicit BoidsUpdateFishPipeline(
        Params params
    );

    ~BoidsUpdateFishPipeline();

    [[nodiscard]]
    bool IsBinded(MFA::RT::CommandRecordState const & recordState) const;

    void BindPipeline(MFA::RT::CommandRecordState & recordState) const;

    void BindDescriptorSets(
        MFA::RT::CommandRecordState const & recordState,
        MFA::RT::DescriptorSetGroup const & fishesDescriptorSets,
        MFA::RT::DescriptorSetGroup const & collisionTrianglesDescriptorSets,
        MFA::RT::DescriptorSetGroup const & simulationConstantsDescriptorSets
    ) const;

    void SetPushConstants(
        MFA::RT::CommandRecordState & recordState,
        PushConstants const & pushConstants
    ) const;

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateFishesDescriptorSets(
        MFA::RT::BufferGroup const & fishBuffer
    ) const;

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateCollisionTrianglesDescriptorSets(
        MFA::RT::BufferGroup const & collisionTriangleBuffer
    ) const;

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateSimulationConstantsDescriptorSets(
        MFA::RT::BufferGroup const & simulationConstantsBuffer
    ) const;

    void Reload() override;

private:

    void CreateDescriptorSetLayouts();

    void CreatePipeline();

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateStorageBufferDescriptorSets(
        MFA::RT::BufferGroup const & bufferGroup,
        MFA::RT::DescriptorSetLayoutGroup const & descriptorSetLayout
    ) const;

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateUniformBufferDescriptorSets(
        MFA::RT::BufferGroup const & bufferGroup,
        MFA::RT::DescriptorSetLayoutGroup const & descriptorSetLayout
    ) const;

    std::shared_ptr<MFA::RT::DescriptorPool> mDescriptorPool{};

    std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> mFishDescriptorSetLayout{};
    std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> mCollisionTriangleDescriptorSetLayout{};
    std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> mSimulationConstantsDescriptorSetLayout{};

    std::shared_ptr<MFA::RT::PipelineGroup> mPipeline{};

    Params mParams{};
};
