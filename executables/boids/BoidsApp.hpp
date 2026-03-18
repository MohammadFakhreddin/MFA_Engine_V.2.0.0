//
// Created by mohammad on 2026-03-08.
//

#ifndef BOID_APP_HPP
#define BOID_APP_HPP

#include "BoidsUpdateFishPipeline.hpp"
#include "RenderTypes.hpp"
#include "SceneRenderPass.hpp"
#include "Time.hpp"
#include "UI.hpp"
#include "camera/ObserverCamera.hpp"
#include "BlinnPhongPipeline.hpp"

#include "BoidsSimulationConstants.hpp"

#include <SDL_events.h>
#include <memory>
#include <optional>

class BoidsSimulationApp
{
public:
    explicit BoidsSimulationApp();

    ~BoidsSimulationApp();

    void Run();

private:

    void Update(float deltaTime);

    void Render(MFA::RT::CommandRecordState &recordState);

    void Resize();

    void Reload();

    void OnSDL_Event(SDL_Event *event);

    void OnUI(float deltaTimeSec);

    void PrepareSceneRenderPass();

    void PrepareUI();

    void PrepareCamera();

    void PrepareFishStorageBuffer();

    void UpdateCamera(float deltaTime);

    void UpdateBufferTrackers(MFA::RT::CommandRecordState const & recordState);

    void ApplyUI_Style();

    void DisplayParametersWindow();

    // You need to be able to select and view objects in the editor window
    void DisplaySceneWindow();

    // Framework params
    std::shared_ptr<MFA::UI> _ui{};
    std::unique_ptr<MFA::Time> _time{};
    std::shared_ptr<MFA::SwapChainRenderResource> _swapChainResource{};
    std::shared_ptr<MFA::DepthRenderResource> _depthResource{};
    std::shared_ptr<MFA::MSSAA_RenderResource> _msaaResource{};
    std::shared_ptr<MFA::DisplayRenderPass> _displayRenderPass{};
    std::shared_ptr<MFA::RT::SamplerGroup> _sampler{};

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

    std::unique_ptr<MFA::ObserverCamera> _camera{};
    std::unique_ptr<MFA::HostVisibleBufferTracker> _cameraBufferTracker{};
    std::unique_ptr<MFA::HostVisibleBufferTracker> _fishStorageBufferTracker{};

    // Rendering params
    std::unique_ptr<MFA::BlinnPhongPipeline> _boidsShadingPipeline{};
    std::unique_ptr<MFA::BlinnPhongPipeline> _environmentShadingPipeline{};
    std::unique_ptr<BoidsUpdateFishPipeline> _boidsUpdateFishPipeline{};

    // Simulation params
    bool _play = true;
    bool _step = false;
    bool _reset = false;

    struct Config
    {
        int fishXCount = 5;	
        int fishYCount = 5;
        int fishZCount = 5;
    
        SimulationConstants simulation;
    };
    Config _config {};
};


#endif // BOID_APP_HPP
