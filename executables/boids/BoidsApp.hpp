//
// Created by mohammad on 2026-03-08.
//

#ifndef BOID_APP_HPP
#define BOID_APP_HPP

#include "AssetGLTF_Mesh.hpp"
#include "BoidsUpdateFishPipeline.hpp"
#include "BufferTracker.hpp"
#include "RenderTypes.hpp"
#include "SceneRenderPass.hpp"
#include "Time.hpp"
#include "UI.hpp"
#include "camera/ObserverCamera.hpp"
#include "BlinnPhongPipeline.hpp"

#include "BoidsCollisionTriangle.hpp"
#include "BoidsSimulationConstants.hpp"

#include <SDL_events.h>
#include <memory>
#include <vector>

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

    void PrepareFishes();

    [[nodiscard]]
    static std::vector<CollisionTriangle> ExtractCollisionTriangles(
        uint32_t vertexCount,
        uint32_t indexCount,
        MFA::AS::GLTF::Vertex * vertices,
        MFA::AS::GLTF::Index * indices
    );

    [[nodiscard]]
    static std::vector<CollisionTriangle> BakeCollisionTriangles(
        std::vector<CollisionTriangle> const & triangles,
        glm::mat4 const & modelMatrix
    );

    void PrepareScene();

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

    struct MeshMetadata
    {
        uint32_t vertexOffset{};
        uint32_t indexOffset{};
        uint32_t indexCount{};
    };

    struct InstanceMetadata
    {
        uint32_t instanceOffset{};
        uint32_t instanceCount{};
    };

    std::unique_ptr<MFA::ObserverCamera> _camera{};
    std::unique_ptr<MFA::HostVisibleBufferTracker> _cameraBufferTracker{};
    std::unique_ptr<MFA::HostVisibleBufferTracker> _fishStorageBufferTracker{};
    std::shared_ptr<MFA::RT::BufferGroup> _sceneVertexBuffer{};
    std::shared_ptr<MFA::RT::BufferGroup> _fishInstanceBuffer{};
    std::shared_ptr<MFA::RT::BufferGroup> _sceneInstanceBuffer{};
    std::shared_ptr<MFA::RT::BufferGroup> _sceneIndexBuffer{};
    std::shared_ptr<MFA::RT::BufferGroup> _sceneCollisionTriangleBuffer{};

    MeshMetadata _fishMeshMetadata{};
    MeshMetadata _cageMeshMetadata{};
    MeshMetadata _torusMeshMetadata{};

    InstanceMetadata _cageInstanceMetadata{};
    InstanceMetadata _torusInstanceMetadata{};

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
