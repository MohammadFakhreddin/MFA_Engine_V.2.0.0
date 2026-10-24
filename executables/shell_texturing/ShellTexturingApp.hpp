//
// Created by mohammad on 2026-03-08.
//

#ifndef MFA_SHELL_TEXTURING_APP_HPP
#define MFA_SHELL_TEXTURING_APP_HPP

#include "BedrockMath.hpp"
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

    void Render(mfa::RT::CommandRecordState &recordState);

    void Resize();

    void Reload();

    void OnSDL_Event(SDL_Event *event);

    void OnUI(float deltaTimeSec);

    void PrepareSceneRenderPass();

    void PrepareMesh();

    void ApplySelectedMesh();

    void PrepareUniformBuffers();

    void PreparePipeline();

    void PrepareDescriptorSets();

    void UpdateCamera(float deltaTime);

    void UpdateBuffers(mfa::RT::CommandRecordState const &recordState);

    void ApplyUI_Style();

    void DisplayParametersWindow();

    void DisplaySceneWindow();

    std::function<VkExtent2D()> const GetWindowSize = [this]() { return _sceneWindowSize; };

    std::function<bool()> const IsWindowFocused = [this]() { return _sceneWindowFocused; };

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

    mfa::ShellTexturingPipeline::Light _light{};

    std::unique_ptr<mfa::HostVisibleBufferTracker> _cameraBufferTracker{};
    std::unique_ptr<mfa::HostVisibleBufferTracker> _shellBufferTracker{};
    std::unique_ptr<mfa::HostVisibleBufferTracker> _modelBufferTracker{};
    std::unique_ptr<mfa::HostVisibleBufferTracker> _materialBufferTracker{};
    std::unique_ptr<mfa::HostVisibleBufferTracker> _lightBufferTracker{};

    std::shared_ptr<mfa::RT::BufferGroup> _vertexBuffer{};
    std::shared_ptr<mfa::RT::BufferGroup> _indexBuffer{};
    std::unique_ptr<mfa::LocalBufferTracker> _vertexBufferTracker{};
    std::unique_ptr<mfa::LocalBufferTracker> _indexBufferTracker{};

    enum class SelectedMesh : int
    {
        Sphere = 0,
        Plane = 1,
        Count = 2
    };
    SelectedMesh _selectedMesh = SelectedMesh::Plane;

    struct MeshSettings
    {
        uint32_t indexCount;
        VkDeviceSize vertexOffset;
        VkDeviceSize indexOffset;

        mfa::ShellTexturingPipeline::Shell shell;
        mfa::ShellTexturingPipeline::Material material;
        mfa::ArcballCamera camera;
        mfa::ShellTexturingPipeline::Model model;
    };
    std::array<MeshSettings, (int)SelectedMesh::Count> _meshSettings{
        {MeshSettings{
             .shell = {.displacement = 0.416f, .radius = 6.818f, .count = 512, .attenuation = 0.79f, .density = 545},
             .material = {.albedo = glm::vec4(79.f / 255.f, 138.f / 255.f, 209.f / 255.f, 1.0f), .albedoTexture = -1},
             .camera = [this]() -> mfa::ArcballCamera
             {
                 using namespace mfa;
                 mfa::ArcballCamera camera(GetWindowSize, IsWindowFocused);
                 glm::vec3 constexpr cameraPosition{0.0f, -3.536f, -3.536f};
                 camera.SetLocalPosition(cameraPosition);
                 camera.SetLocalQuaternion(glm::quatLookAt(glm::normalize(cameraPosition), Math::UpVec3));
                 camera.SetminDistance(2.0f);
                 camera.SetmaxDistance(12.0f);
                 return camera;
             }(),
             .model = {.model = glm::mat4(1.0f)}},
         MeshSettings{
             .shell = {.displacement = 0.156f, .radius = 3.065f, .count = 512, .attenuation = 0.99f, .density = 278},
             .material = {.albedo = glm::vec4(46.f / 255.f, 242.f / 255.f, 64.f / 255.f, 1.0f), .albedoTexture = -1},
             .camera = [this]() -> mfa::ArcballCamera
             {
                 using namespace mfa;
                 mfa::ArcballCamera camera(GetWindowSize, IsWindowFocused);
                 glm::vec3 constexpr cameraPosition{-1.894f, -2.179f, -2.768f};
                 camera.SetLocalPosition(cameraPosition);
                 camera.SetLocalQuaternion(glm::quatLookAt(glm::normalize(cameraPosition), Math::UpVec3));
                 camera.SetminDistance(2.0f);
                 camera.SetmaxDistance(12.0f);
                 return camera;
             }(),
             .model = {.model = glm::mat4(1.0f)}}}};

    std::shared_ptr<mfa::RT::GpuTexture> _albedoTexture{};

    std::unique_ptr<mfa::ShellTexturingPipeline> _pipeline{};
    mfa::RT::DescriptorSetGroup _cameraDescriptorSets{};
    mfa::RT::DescriptorSetGroup _shellDescriptorSets{};
    mfa::RT::DescriptorSetGroup _modelDescriptorSets{};
    mfa::RT::DescriptorSetGroup _materialDescriptorSets{};
    mfa::RT::DescriptorSetGroup _albedoTextureDescriptorSet{};
    mfa::RT::DescriptorSetGroup _lightDescriptorSets{};
};


#endif // MFA_SHELL_TEXTURING_APP_HPP
