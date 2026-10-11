#pragma once

#include "RenderTypes.hpp"

class SceneRenderResource
{
public:

    explicit SceneRenderResource(
        VkExtent2D imageExtent,
        VkFormat imageFormat,
        VkFormat depthFormat,
        VkSampleCountFlagBits msaaSampleCount
    );
    ~SceneRenderResource();

    [[nodiscard]]
    VkExtent2D const & ImageExtent() const {return _imageExtent;}

    [[nodiscard]]
    VkFormat ImageFormat() const {return _imageFormat;}

    [[nodiscard]]
    VkSampleCountFlagBits MSAA_SampleCount() const {return _msaaSampleCount;}

    [[nodiscard]]
    mfa::RT::ColorImageGroup const & MSAA_Image(mfa::RT::CommandRecordState const & recordState) const
    {
        return *_msaaImageList[recordState.imageIndex];
    }

    [[nodiscard]]
    mfa::RT::ColorImageGroup const & MSAA_Image(int const imageIndex) const
    {
        return *_msaaImageList[imageIndex];
    }

    [[nodiscard]]
    mfa::RT::ColorImageGroup const & ColorImage(mfa::RT::CommandRecordState const & recordState) const
    {
        return *_colorImageList[recordState.imageIndex];
    }

    [[nodiscard]]
    mfa::RT::ColorImageGroup const & ColorImage(int const imageIndex) const
    {
        return *_colorImageList[imageIndex];
    }

    [[nodiscard]]
    mfa::RT::DepthImageGroup const & DepthImage(mfa::RT::CommandRecordState const & recordState) const
    {
        return *_depthImageList[recordState.imageIndex];
    }

    [[nodiscard]]
    mfa::RT::DepthImageGroup const & DepthImage(int const imageIndex) const
    {
        return *_depthImageList[imageIndex];
    }

private:

    VkExtent2D const _imageExtent;
    VkFormat const _imageFormat;
    VkFormat const _depthFormat;
    VkSampleCountFlagBits _msaaSampleCount;

    std::vector<std::shared_ptr<mfa::RT::ColorImageGroup>> _msaaImageList;
    std::vector<std::shared_ptr<mfa::RT::ColorImageGroup>> _colorImageList;
    std::vector<std::shared_ptr<mfa::RT::DepthImageGroup>> _depthImageList;

};
