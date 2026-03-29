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

    void BindFishes(
        MFA::RT::CommandRecordState const & recordState,
        MFA::RT::DescriptorSetGroup const & fishesDescriptorSets
    ) const;

    void BindCollisionTriangles(
        MFA::RT::CommandRecordState const & recordState,
        MFA::RT::DescriptorSetGroup const & collisionTrianglesDescriptorSets
    ) const;

    void BindSimulationConstants(
        MFA::RT::CommandRecordState const & recordState,
        MFA::RT::DescriptorSetGroup const & simulationConstantsDescriptorSets
    ) const;

    void SetPushConstants(
        MFA::RT::CommandRecordState & recordState,
        PushConstants const & pushConstants
    ) const;

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateFishDescriptorSets(
        MFA::RT::BufferGroup const & stateBuffer,                           // Stores the boids state
        MFA::RT::BufferGroup const & instanceBuffer                         // Stores the final transform used for rendering
    ) const;

    void UpdateFishDescriptorSets(
        MFA::RT::DescriptorSetGroup & descriptorSetGroup,
        MFA::RT::BufferGroup const & stateBuffer,                           // Stores the boids state
        MFA::RT::BufferGroup const & instanceBuffer                         // Stores the final transform used for rendering
    ) const;

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateCollisionTrianglesDescriptorSets(
        MFA::RT::BufferGroup const & collisionTriangleBuffer                // Collision triangle buffer
    );

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateSimulationConstantsDescriptorSets(
        MFA::RT::BufferGroup const & simulationConstantsBuffer              // Simulation constants buffer
    );

    void Reload() override;

private:

    void CreateDescriptorSetLayouts();

    void CreatePipeline();

    std::shared_ptr<MFA::RT::DescriptorPool> mDescriptorPool{};

    std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> mFishDescriptorSetLayout{};
    std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> mCollisionTriangleDescriptorSetLayout{};
    std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> mSimulationConstantsDescriptorSetLayout{};

    std::shared_ptr<MFA::RT::PipelineGroup> mPipeline{};

    Params mParams{};
};
