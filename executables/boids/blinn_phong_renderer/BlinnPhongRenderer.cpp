#include "BlinnPhongRenderer.hpp"

#include "LogicalDevice.hpp"

#include <utility>

using namespace MFA;

//----------------------------------------------------------------------------------------------------------------------

BlinnPhongRenderer::BlinnPhongRenderer(
    std::shared_ptr<Pipeline> pipeline,

    std::shared_ptr<RT::BufferGroup> viewProjectionBuffer,
    std::shared_ptr<RT::BufferGroup> lightSourceBuffer,
    // Vertex count
    int const vertexCount,
    Vertex const *vertices,
    Normal const *normals,
    // Index count
    int const indexCount,
    Index const *indices,

    VkCommandBuffer const & commandBuffer,
    std::shared_ptr<MFA::RT::BufferGroup> & vertexStageBuffer,
    std::shared_ptr<MFA::RT::BufferGroup> & indexStageBuffer,

    int const maxInstanceCount
) :
    mPipeline(std::move(pipeline)),
    mViewProjectionBuffer(std::move(viewProjectionBuffer)),
    mLightSourceBuffer(std::move(lightSourceBuffer)),
    mVertexCount(vertexCount),
    mIndexCount(indexCount),
    mMaxInstanceCount(maxInstanceCount)
{
    vertexStageBuffer = GenerateVertexBuffer(commandBuffer, mVertexCount, vertices, normals);
    indexStageBuffer = GenerateIndexBuffer(commandBuffer, mIndexCount, indices);

    mDescriptorSetGroup = mPipeline->CreatePerRenderDescriptorSets(*mViewProjectionBuffer, *mLightSourceBuffer);

    mInstanceBuffer = std::make_shared<HostVisibleBufferTracker>(RB::CreateBufferGroup(
        LogicalDevice::GetVkDevice(),
        LogicalDevice::GetPhysicalDevice(),
        sizeof(Pipeline::Instance) * maxInstanceCount,
        LogicalDevice::GetMaxFramePerFlight(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    ));
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongRenderer::Queue(Pipeline::Instance const & instance)
{
    auto * data = mInstanceBuffer->Data();
    data += mInstanceCount * sizeof(Pipeline::Instance);
    memcpy(data, &instance, sizeof(Pipeline::Instance));
    mInstanceCount += 1;
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongRenderer::Render(RT::CommandRecordState &recordState)
{
    mInstanceBuffer->Update(recordState);

    mPipeline->BindPipeline(recordState);

    RB::AutoBindDescriptorSet(recordState, RB::UpdateFrequency::PerPipeline, mDescriptorSetGroup);

    RB::BindIndexBuffer(
        recordState,
        *_indexBuffer,
        0,
        VK_INDEX_TYPE_UINT32
    );

    RB::BindVertexBuffer(
        recordState,
        *_vertexBuffer,
        0,
        0
    );

    RB::BindVertexBuffer(
        recordState,
        *mInstanceBuffer->HostVisibleBuffer().buffers[recordState.frameIndex],
        1,
        0
    );

    RB::DrawIndexed(
        recordState,
        mIndexCount,
        mInstanceCount,
        0
    );

    mInstanceCount = 0;
}

//----------------------------------------------------------------------------------------------------------------------

std::shared_ptr<RT::BufferGroup> BlinnPhongRenderer::GenerateVertexBuffer(
    VkCommandBuffer cb,
    int const vertexCount,
    Vertex const * vertices,
    Normal const * normals
)
{
    std::vector<Pipeline::Vertex> cpuVertices(vertexCount);

    for (int i = 0; i < mVertexCount; ++i)
    {
        cpuVertices[i] = Pipeline::Vertex
        {
            .position = vertices[i],
            .normal = normals[i]
        };
    }

    auto const alias = Alias(cpuVertices.data(), cpuVertices.size());

    auto const stageBuffer = RB::CreateStageBuffer(
        LogicalDevice::GetVkDevice(),
        LogicalDevice::GetPhysicalDevice(),
        alias.Len(),
        1
    );

    _vertexBuffer = RB::CreateVertexBuffer(
        LogicalDevice::GetVkDevice(),
        LogicalDevice::GetPhysicalDevice(),
        cb,
        *stageBuffer->buffers[0],
        alias
    );

    return stageBuffer;
}

//----------------------------------------------------------------------------------------------------------------------

std::shared_ptr<RT::BufferGroup> BlinnPhongRenderer::GenerateIndexBuffer(
    VkCommandBuffer cb,
    int const indexCount,
    Index const * indices
)
{
    Alias const indexBlob {indices, sizeof(Index) * indexCount};

    auto const indexStageBuffer = RB::CreateStageBuffer(
        LogicalDevice::GetVkDevice(),
        LogicalDevice::GetPhysicalDevice(),
        indexBlob.Len(),
        1
    );

    _indexBuffer = RB::CreateIndexBuffer(
        LogicalDevice::GetVkDevice(),
        LogicalDevice::GetPhysicalDevice(),
        cb,
        *indexStageBuffer->buffers[0],
        indexBlob
    );

    return indexStageBuffer;
}

//----------------------------------------------------------------------------------------------------------------------


