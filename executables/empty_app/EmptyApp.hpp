//
// Created by mohammad on 2026-03-08.
//

#ifndef VOLUMETRICCLOUD_PERLINNOISEAPP_HPP
#define VOLUMETRICCLOUD_PERLINNOISEAPP_HPP

#include "RenderTypes.hpp"
#include "SceneRenderPass.hpp"
#include "Time.hpp"
#include "UI.hpp"

#include <SDL_events.h>

// TODO: This scene is used to generate, visualize and save perlin noise for cloud texture.
class EmptyApp
{
public:
    explicit EmptyApp();

    ~EmptyApp();

    void Run();

private:

    void Update(float deltaTime);

    void Render(mfa::RT::CommandRecordState &recordState);

    void Resize();

    void Reload();

    void OnSDL_Event(SDL_Event *event);

    void OnUI(float deltaTimeSec);

    void PrepareSceneRenderPass();

    void ApplyUI_Style();

    void DisplayParametersWindow();

    // You need to be able to select and view objects in the editor window
    void DisplaySceneWindow();

    std::shared_ptr<mfa::UI> _ui{};
    std::unique_ptr<mfa::Time> _time{};
    std::shared_ptr<mfa::SwapChainRenderResource> _swapChainResource{};
    std::shared_ptr<mfa::DepthRenderResource> _depthResource{};
    std::shared_ptr<mfa::MSSAA_RenderResource> _msaaResource{};
    std::shared_ptr<mfa::DisplayRenderPass> _displayRenderPass{};
    std::shared_ptr<mfa::RT::SamplerGroup> _sampler{};

    std::shared_ptr<SceneFrameBuffer> _sceneFrameBuffer{};
    std::shared_ptr<SceneRenderResource> _sceneRenderResource{};
    std::shared_ptr<SceneRenderPass> _sceneRenderPass{};

    std::vector<ImTextureID> _sceneTextureID_List{};
    VkExtent2D _sceneWindowSize{800, 800};
    bool _sceneWindowResized = false;
    bool _sceneWindowFocused = false;

    ImFont *_defaultFont{};
    ImFont *_boldFont{};

    int _activeImageIndex{};
};


#endif // VOLUMETRICCLOUD_PERLINNOISEAPP_HPP
