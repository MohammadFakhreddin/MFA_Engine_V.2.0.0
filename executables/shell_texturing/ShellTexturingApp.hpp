//
// Created by mohammad on 2026-03-08.
//

#ifndef MFA_SHELL_TEXTURING_APP_HPP
#define MFA_SHELL_TEXTURING_APP_HPP

#include "BufferTracker.hpp"
#include "RenderTypes.hpp"
#include "SceneRenderPass.hpp"
#include "ShellTexturingPipeline.hpp"
#include "Time.hpp"
#include "UI.hpp"
#include "camera/ArcballCamera.hpp"

#include <SDL_events.h>

class ShellTexturingApp
{
public:
    explicit ShellTexturingApp();

    ~ShellTexturingApp();

    void Run();

private:

    void Update(float deltaTime);

    void Render(MFA::RT::CommandRecordState &recordState);

    void Resize();

    void Reload();

    void OnSDL_Event(SDL_Event *event);

    void OnUI(float deltaTimeSec);

    void PrepareSceneRenderPass();

    void PrepareCamera();

    void PrepareMesh();

    void PrepareUniformBuffers();

    void PreparePipeline();

    void PrepareDescriptorSets();

    void UpdateCamera(float deltaTime);

    void UpdateBuffers(MFA::RT::CommandRecordState const &recordState);

    void ApplyUI_Style();

    void DisplayParametersWindow();

    void DisplaySceneWindow();

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

    std::unique_ptr<MFA::ArcballCamera> _camera{};

    MFA::ShellTexturingPipeline::Shell _shell{};
    MFA::ShellTexturingPipeline::Model _model{};
    MFA::ShellTexturingPipeline::Material _material{};
    MFA::ShellTexturingPipeline::Light _light{};

    std::unique_ptr<MFA::HostVisibleBufferTracker> _cameraBufferTracker{};
    std::unique_ptr<MFA::HostVisibleBufferTracker> _shellBufferTracker{};
    std::unique_ptr<MFA::HostVisibleBufferTracker> _modelBufferTracker{};
    std::unique_ptr<MFA::HostVisibleBufferTracker> _materialBufferTracker{};
    std::unique_ptr<MFA::HostVisibleBufferTracker> _lightBufferTracker{};

    std::shared_ptr<MFA::RT::BufferGroup> _vertexBuffer{};
    std::shared_ptr<MFA::RT::BufferGroup> _indexBuffer{};
    std::unique_ptr<MFA::LocalBufferTracker> _vertexBufferTracker{};
    std::unique_ptr<MFA::LocalBufferTracker> _indexBufferTracker{};
    uint32_t _indexCount = 0;
    int _sliceCount = 64;
    int _stackCount = 48;
    bool _meshRebuildRequested = false;

    std::shared_ptr<MFA::RT::GpuTexture> _albedoTexture{};

    std::unique_ptr<MFA::ShellTexturingPipeline> _pipeline{};
    MFA::RT::DescriptorSetGroup _cameraDescriptorSets{};
    MFA::RT::DescriptorSetGroup _shellDescriptorSets{};
    MFA::RT::DescriptorSetGroup _modelDescriptorSets{};
    MFA::RT::DescriptorSetGroup _materialDescriptorSets{};
    MFA::RT::DescriptorSetGroup _albedoTextureDescriptorSet{};
    MFA::RT::DescriptorSetGroup _lightDescriptorSets{};
};


#endif // MFA_SHELL_TEXTURING_APP_HPP
