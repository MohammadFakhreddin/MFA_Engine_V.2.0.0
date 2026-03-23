#include "BufferTracker.hpp"

#include "BedrockAssert.hpp"
#include "LogicalDevice.hpp"

namespace MFA
{
    namespace
    {
        struct BufferBarrierInfo
        {
            VkPipelineStageFlags dstStageMask = 0;
            VkAccessFlags dstAccessMask = 0;
        };

        [[nodiscard]]
        BufferBarrierInfo InferLocalBufferBarrier(
            RT::CommandBufferType const commandBufferType,
            RT::BufferGroup const & bufferGroup
        )
        {
            BufferBarrierInfo info{};
            auto const usageFlags = bufferGroup.bufferUsageFlags;

            if (commandBufferType == RT::CommandBufferType::Graphic)
            {
                if ((usageFlags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) != 0)
                {
                    info.dstStageMask |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                    info.dstAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                }
                if ((usageFlags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) != 0)
                {
                    info.dstStageMask |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                    info.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
                }
                if ((usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) != 0)
                {
                    info.dstStageMask |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    info.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
                }
                if ((usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0)
                {
                    info.dstStageMask |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    info.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                }
                if ((usageFlags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) != 0)
                {
                    info.dstStageMask |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
                    info.dstAccessMask |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                }
            }
            else if (commandBufferType == RT::CommandBufferType::Compute)
            {
                if ((usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) != 0)
                {
                    info.dstStageMask |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    info.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
                }
                if ((usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0)
                {
                    info.dstStageMask |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    info.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                }
            }

            return info;
        }
    } // namespace

    //-----------------------------------------------------------------------------------------------

    HostVisibleBufferTracker::HostVisibleBufferTracker(
        std::shared_ptr<RT::BufferGroup> bufferGroup, 
        Alias const & data
    ) : HostVisibleBufferTracker(std::move(bufferGroup))
    {
        for (auto const& buffer : mBufferGroup->buffers)
        {
            RB::UpdateHostVisibleBuffer(
                LogicalDevice::GetVkDevice(),
                *buffer,
                Alias(data.Ptr(), data.Len())
            );
        }
    }
    
    //-----------------------------------------------------------------------------------------------
    
    HostVisibleBufferTracker::HostVisibleBufferTracker(std::shared_ptr<RT::BufferGroup> bufferGroup)
        : mBufferGroup(std::move(bufferGroup))
    {
        mData = Memory::AllocSize(mBufferGroup->bufferSize);
        mDirtyCounter = 0;
    }

    //-----------------------------------------------------------------------------------------------
    
    void HostVisibleBufferTracker::Update(RT::CommandRecordState const & recordState)
    {
        if (mDirtyCounter > 0)
        {
            MFA_ASSERT(mData != nullptr && mData->IsValid());
            RB::UpdateHostVisibleBuffer(
                LogicalDevice::GetVkDevice(),
                *mBufferGroup->buffers[recordState.frameIndex % mBufferGroup->buffers.size()],
                Alias(mData->Ptr(), mData->Len())
            );
            --mDirtyCounter;
        }
    }

    //-----------------------------------------------------------------------------------------------

    void HostVisibleBufferTracker::SetData(Alias const & data)
    {
        mDirtyCounter = (int)mBufferGroup->buffers.size();
        MFA_ASSERT(data.Len() <= mData->Len());
        std::memcpy(mData->Ptr(), data.Ptr(), data.Len());
    }

    //-----------------------------------------------------------------------------------------------

    uint8_t * HostVisibleBufferTracker::Data()
    {
        mDirtyCounter = (int)mBufferGroup->buffers.size();
        return mData->Ptr();
    }

    //-----------------------------------------------------------------------------------------------

    RT::BufferGroup const & HostVisibleBufferTracker::HostVisibleBuffer() const
    {
        return *mBufferGroup;
    }

    //-----------------------------------------------------------------------------------------------

    LocalBufferTracker::LocalBufferTracker(
        std::shared_ptr<RT::BufferGroup> localBuffer,
        std::shared_ptr<RT::BufferGroup> hostVisibleBuffer,
        Alias const & data
    )
        : mLocalBuffer(std::move(localBuffer))
        , mHostVisibleBuffer(std::move(hostVisibleBuffer))
    {
        mData = Memory::AllocSize(mLocalBuffer->bufferSize);
        SetData(data);
    }

    //-----------------------------------------------------------------------------------------------

    LocalBufferTracker::LocalBufferTracker(
        std::shared_ptr<RT::BufferGroup> localBuffer,
        std::shared_ptr<RT::BufferGroup> hostVisibleBuffer
    )
        : mLocalBuffer(std::move(localBuffer))
        , mHostVisibleBuffer(std::move(hostVisibleBuffer))
    {
        mData = Memory::AllocSize(mLocalBuffer->bufferSize);
    }

    //-----------------------------------------------------------------------------------------------

    void LocalBufferTracker::Update(RT::CommandRecordState const & recordState)
    {
        auto const hostVisibleBufferIndex = recordState.frameIndex % mHostVisibleBuffer->buffers.size();
        auto const localBufferIndex = recordState.frameIndex % mLocalBuffer->buffers.size();

        if (mHostVisibleDirtyCounter > 0)
        {
            MFA_ASSERT(mData != nullptr && mData->IsValid());
            RB::UpdateHostVisibleBuffer(
                LogicalDevice::GetVkDevice(),
                *mHostVisibleBuffer->buffers[hostVisibleBufferIndex],
                Alias(mData->Ptr(), mData->Len())
            );
            --mHostVisibleDirtyCounter;
        }

        if (mLocalDirtyCounter > 0)
        {
            MFA_ASSERT(recordState.commandBuffer != nullptr);
            MFA_ASSERT(mData != nullptr && mData->IsValid());

            auto const & localBuffer = *mLocalBuffer->buffers[localBufferIndex];
            auto const & hostVisibleBuffer = *mHostVisibleBuffer->buffers[hostVisibleBufferIndex];

            RB::UpdateLocalBuffer(
                recordState.commandBuffer,
                localBuffer,
                hostVisibleBuffer
            );

            auto const barrierInfo = InferLocalBufferBarrier(recordState.commandBufferType, *mLocalBuffer);
            if (barrierInfo.dstStageMask != 0 && barrierInfo.dstAccessMask != 0)
            {
                VkBufferMemoryBarrier barrier{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstAccessMask = barrierInfo.dstAccessMask,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = localBuffer.buffer,
                    .offset = 0,
                    .size = localBuffer.size,
                };
                RB::PipelineBarrier(
                    recordState.commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    barrierInfo.dstStageMask,
                    1,
                    &barrier
                );
            }

            --mLocalDirtyCounter;
        }
    }

    //-----------------------------------------------------------------------------------------------

    void LocalBufferTracker::SetData(Alias const & data)
    {
        MFA_ASSERT(data.Len() <= mData->Len());
        mLocalDirtyCounter = (int)mLocalBuffer->buffers.size();
        mHostVisibleDirtyCounter = (int)mHostVisibleBuffer->buffers.size();
        std::memcpy(mData->Ptr(), data.Ptr(), data.Len());
    }

    //-----------------------------------------------------------------------------------------------

    uint8_t * LocalBufferTracker::Data()
    {
        // Returns the data and resets the counter.
        mLocalDirtyCounter = (int)mLocalBuffer->buffers.size();
        mHostVisibleDirtyCounter = (int)mHostVisibleBuffer->buffers.size();
        return mData->Ptr();
    }

    //-----------------------------------------------------------------------------------------------

    RT::BufferGroup const & LocalBufferTracker::HostVisibleBuffer() const
    {
        return *mHostVisibleBuffer;
    }

    //-----------------------------------------------------------------------------------------------

    RT::BufferGroup const & LocalBufferTracker::LocalBuffer() const
    {
        return *mLocalBuffer;
    }
    
    //-----------------------------------------------------------------------------------------------
    
}
