#pragma once

#include "pipeline/IShadingPipeline.hpp"
#include "RenderTypes.hpp"

#include <cstdint>

class BoidsUpdateFishPipeline : public mfa::IShadingPipeline
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
    bool IsBinded(mfa::RT::CommandRecordState const & recordState) const;

    void BindPipeline(mfa::RT::CommandRecordState & recordState) const;

    void BindFishes(
        mfa::RT::CommandRecordState const & recordState,
        mfa::RT::DescriptorSetGroup const & fishesDescriptorSets
    ) const;

    void BindCollisionTriangles(
        mfa::RT::CommandRecordState const & recordState,
        mfa::RT::DescriptorSetGroup const & collisionTrianglesDescriptorSets
    ) const;

    void BindSimulationConstants(
        mfa::RT::CommandRecordState const & recordState,
        mfa::RT::DescriptorSetGroup const & simulationConstantsDescriptorSets
    ) const;

    void SetPushConstants(
        mfa::RT::CommandRecordState & recordState,
        PushConstants const & pushConstants
    ) const;

    [[nodiscard]]
    mfa::RT::DescriptorSetGroup CreateFishDescriptorSets(
        mfa::RT::BufferGroup const & stateBuffer,                           // Stores the boids state
        mfa::RT::BufferGroup const & instanceBuffer                         // Stores the final transform used for rendering
    ) const;

    void UpdateFishDescriptorSets(
        mfa::RT::DescriptorSetGroup & descriptorSetGroup,
        mfa::RT::BufferGroup const & stateBuffer,                           // Stores the boids state
        mfa::RT::BufferGroup const & instanceBuffer                         // Stores the final transform used for rendering
    ) const;

    [[nodiscard]]
    mfa::RT::DescriptorSetGroup CreateCollisionTrianglesDescriptorSets(
        mfa::RT::BufferGroup const & collisionTriangleBuffer                // Collision triangle buffer
    );

    [[nodiscard]]
    mfa::RT::DescriptorSetGroup CreateSimulationConstantsDescriptorSets(
        mfa::RT::BufferGroup const & simulationConstantsBuffer              // Simulation constants buffer
    );

    void Reload() override;

private:

    void CreateDescriptorSetLayouts();

    void CreatePipeline();

    std::shared_ptr<mfa::RT::DescriptorPool> mDescriptorPool{};

    std::shared_ptr<mfa::RT::DescriptorSetLayoutGroup> mFishDescriptorSetLayout{};
    std::shared_ptr<mfa::RT::DescriptorSetLayoutGroup> mCollisionTriangleDescriptorSetLayout{};
    std::shared_ptr<mfa::RT::DescriptorSetLayoutGroup> mSimulationConstantsDescriptorSetLayout{};

    std::shared_ptr<mfa::RT::PipelineGroup> mPipeline{};

    Params mParams{};
};
