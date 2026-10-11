#include "RenderResource.hpp"

#include "LogicalDevice.hpp"

namespace mfa
{

    //-------------------------------------------------------------------------------------------------

    RenderResource::RenderResource()
    {
        mResizeEventId = LogicalDevice::ResizeEventSignal1.Register([this]()->void
        {
            OnResize();
        });
    }

    //-------------------------------------------------------------------------------------------------

    RenderResource::~RenderResource()
    {
        LogicalDevice::ResizeEventSignal1.UnRegister(mResizeEventId);
    }

    //-------------------------------------------------------------------------------------------------

}
