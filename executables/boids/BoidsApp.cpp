//
// Created by mohammad on 2026-03-08.
//

#include "BoidsApp.hpp"

#include "AssetGLTF_Model.hpp"
#include "BedrockAssert.hpp"
#include "BedrockMath.hpp"
#include "BedrockPath.hpp"
#include "BlinnPhongPipeline.hpp"
#include "BoidsCollisionTriangle.hpp"
#include "BoidsFish.hpp"
#include "BoidsSimulationConstants.hpp"
#include "BufferTracker.hpp"
#include "ImportGLTF.hpp"
#include "JobSystem.hpp"
#include "LogicalDevice.hpp"
#include "RenderBackend.hpp"
#include "camera/ArcballCamera.hpp"
#include "ext/matrix_transform.hpp"
#include "vulkan/vulkan_core.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>

using namespace MFA;

namespace
{
    [[nodiscard]] BlinnPhongPipeline::LightSource ToGpu(BlinnPhongLight const &light)
    {
        auto direction = light.direction;
        auto const directionLength2 = glm::dot(direction, direction);
        if (directionLength2 > 0.0f)
        {
            direction /= std::sqrt(directionLength2);
        }

        return BlinnPhongPipeline::LightSource{.direction = direction,
                                               .ambientStrength = light.ambientStrength,
                                               .color = glm::vec3(light.color) * light.intensity};
    }
} // namespace

//======================================================================================================================

BoidsSimulationApp::BoidsSimulationApp()
{
    if (SDL_JoystickOpen(0) != nullptr)
        SDL_JoystickEventState(SDL_ENABLE);

    LogicalDevice::SDL_EventSignal.Register([&](SDL_Event *event) -> void { OnSDL_Event(event); });

    _swapChainResource = std::make_shared<SwapChainRenderResource>();
    _depthResource = std::make_shared<DepthRenderResource>();
    _msaaResource = std::make_shared<MSSAA_RenderResource>();
    _displayRenderPass = std::make_shared<DisplayRenderPass>(_swapChainResource, _depthResource, _msaaResource);

    _sampler = RB::CreateSampler(LogicalDevice::GetVkDevice(), RB::CreateSamplerParams{});

    PrepareUI();

    LogicalDevice::ResizeEventSignal2.Register([this]() -> void { Resize(); });

    PrepareSceneRenderPass();

    PrepareCamera();
    PrepareSimulationConstants();
    PrepareLighting();
    PrepareScene();
    PreparePipelines();
    PrepareRenderDescriptorSets();
    PrepareFishes();
    PrepareComputeDescriptorSets();
}

//======================================================================================================================

BoidsSimulationApp::~BoidsSimulationApp() = default;

//======================================================================================================================

void BoidsSimulationApp::PrepareSimulationConstants()
{
    _config.fishXCount = 5;
    _config.fishYCount = 5;
    _config.fishZCount = 5;

    _config.simulation.bEnableSeparationForce = true;
    _config.simulation.bEnableAlignmentForce = false;
    _config.simulation.bEnableCohesionForce = false;
    _config.simulation.bEnableSoftCollisionHandling = false;
    _config.simulation.bEnableHardCollisionHandling = false;
    
    _config.simulation.separationRadius = 2.0f;
    _config.simulation.alignmentRadius = 4.0f;
    _config.simulation.cohesionRadius = 4.0f;

    _config.simulation.separationCos = -0.9848077f;
    _config.simulation.alignmentCos = 0.0f;
    _config.simulation.cohesionCos = 0.0f;

    _config.simulation.separationConstant = 10.0f;
    _config.simulation.cohesionConstant = 0.25f;
    _config.simulation.alignmentConstant = 0.25f;

    _config.simulation.bClampSpeed = true;
    _config.simulation.minSpeed = 5.0f;
    _config.simulation.maxSpeed = 10.0f;

    _config.simulation.bClampAcc = false;
    _config.simulation.minAcc = 0.0f;
    _config.simulation.maxAcc = 20.0f;

    _config.simulation.softCollisionOffset = 0.4f;
    _config.simulation.hardCollisionOffset = 0.4f;

    auto simulationConstantsBuffer =
        RB::CreateLocalUniformBuffer(LogicalDevice::GetVkDevice(), LogicalDevice::GetPhysicalDevice(),
                                     sizeof(SimulationConstants), LogicalDevice::GetMaxFramePerFlight());
    auto simulationConstantsStageBuffer = RB::CreateStageBuffer(
        LogicalDevice::GetVkDevice(), LogicalDevice::GetPhysicalDevice(), sizeof(SimulationConstants), 1);
    _simulationConstantsBufferTracker = std::make_unique<LocalBufferTracker>(
        simulationConstantsBuffer, simulationConstantsStageBuffer, Alias{_config.simulation});
}

//======================================================================================================================

void BoidsSimulationApp::PrepareLighting()
{
    _light = BlinnPhongLight{};

    auto lightBuffer =
        RB::CreateLocalUniformBuffer(LogicalDevice::GetVkDevice(), LogicalDevice::GetPhysicalDevice(),
                                     sizeof(BlinnPhongPipeline::LightSource), LogicalDevice::GetMaxFramePerFlight());
    auto lightStageBuffer = RB::CreateStageBuffer(LogicalDevice::GetVkDevice(), LogicalDevice::GetPhysicalDevice(),
                                                  sizeof(BlinnPhongPipeline::LightSource), 1);
    auto lightData = ToGpu(_light);
    _lightBufferTracker = std::make_unique<LocalBufferTracker>(lightBuffer, lightStageBuffer, Alias{lightData});
}

//======================================================================================================================

void BoidsSimulationApp::PrepareUI()
{
    _ui = std::make_shared<UI>(_displayRenderPass,
                               UI::Params{.lightMode = false,
                                          .fontCallback = [this](ImGuiIO &io) -> void
                                          {
                                              { // Default font
                                                  auto const fontPath =
                                                      Path::Get("fonts/JetBrains-Mono/JetBrainsMonoNL-Regular.ttf");
                                                  MFA_ASSERT(std::filesystem::exists(fontPath));
                                                  _defaultFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 20.0f);
                                                  MFA_ASSERT(_defaultFont != nullptr);
                                              }
                                              { // Bold font
                                                  auto const fontPath =
                                                      Path::Get("fonts/JetBrains-Mono/JetBrainsMono-Bold.ttf");
                                                  MFA_ASSERT(std::filesystem::exists(fontPath));
                                                  _boldFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 20.0f);
                                                  MFA_ASSERT(_boldFont != nullptr);
                                              }
                                          }});
    _ui->UpdateSignal.Register([this]() -> void { OnUI(Time::DeltaTimeSec()); });
}

//======================================================================================================================

void BoidsSimulationApp::PrepareCamera()
{
    _camera = std::make_unique<MFA::ArcballCamera>([this]() -> VkExtent2D { return _sceneWindowSize; },
                                                    [this]() -> bool { return _sceneWindowFocused; });

    
    _camera->SetLocalPosition(glm::vec3{-54.0f/2, -36.0f/2, 69.0f/2});
    _camera->SetmaxDistance(100.0f);
    
    _camera->Update(1.0f / 120.0f);

    auto const globalPosition = _camera->GlobalPosition();
    auto const viewProjection = _camera->ViewProjection();

    auto cameraBuffer = RB::CreateHostVisibleUniformBuffer(
        LogicalDevice::GetVkDevice(), 
        LogicalDevice::GetPhysicalDevice(),
        sizeof(BlinnPhongPipeline::Camera), 
        LogicalDevice::GetMaxFramePerFlight()
    );

    _cameraBufferTracker = std::make_unique<HostVisibleBufferTracker>(cameraBuffer);

    BlinnPhongPipeline::Camera cameraData{
        .viewProjection = viewProjection,
        .position = globalPosition
    };

    _cameraBufferTracker->SetData(Alias{cameraData});
}

//======================================================================================================================

void BoidsSimulationApp::PrepareFishes()
{
    auto const spawnBoids = [this]() -> std::vector<FishState>
    {
        std::vector<FishState> boids{};

        float const spawnRadius = 5.0f;

        float const speedMin = -10.0f;
        float const speedMax = 10.0f;

        auto const normalizedIndex = [](int index, int count) -> float
        {
            if (count <= 0)
            {
                return 0.0f;
            }
            return static_cast<float>(index) / static_cast<float>(count);
        };

        for (int xIdx = 0; xIdx <= _config.fishXCount; ++xIdx)
        {
            for (int yIdx = 0; yIdx <= _config.fishYCount; ++yIdx)
            {
                for (int zIdx = 0; zIdx <= _config.fishZCount; ++zIdx)
                {
                    FishState boid{};
                    boid.id = static_cast<int>(boids.size());

                    float const xPer = normalizedIndex(xIdx, _config.fishXCount);
                    float const yPer = normalizedIndex(yIdx, _config.fishYCount);
                    float const zPer = normalizedIndex(zIdx, _config.fishZCount);

                    float const xPos = xPer * spawnRadius * 2.0f - spawnRadius;
                    float const yPos = yPer * spawnRadius * 2.0f - spawnRadius;
                    float const zPos = zPer * spawnRadius * 2.0f - spawnRadius;

                    boid.rbPosition = glm::vec3{xPos, yPos, zPos};
                    boid.tPosition = boid.rbPosition;
                    boid.tRotation = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                    boid.tScale = glm::vec3(0.035f);

                    auto const velX = Math::Random<float>(speedMin, speedMax);
                    auto const velY = Math::Random<float>(speedMin, speedMax);
                    auto const velZ = Math::Random<float>(speedMin, speedMax);
                    boid.rbVelocity = glm::vec3{velX, velY, velZ};
                    
                    boid.tLocalMat4 = glm::identity<glm::mat4>();
                    
                    boids.emplace_back(boid);
                }
            }
        }

        return boids;
    };

    auto const boids = spawnBoids();

    _fishInstanceMetadata.instanceOffset = 0;
    _fishInstanceMetadata.instanceCount = static_cast<int>(boids.size());

    std::vector<glm::mat4> fishModels{};
    fishModels.reserve(boids.size());
    std::vector<int32_t> fishMaterialIds{};
    fishMaterialIds.reserve(boids.size());
    for (auto const & boid : boids)
    {
        auto const model = glm::translate(glm::identity<glm::mat4>(), boid.tPosition) 
            * glm::scale(glm::identity<glm::mat4>(), boid.tScale) 
            * boid.tLocalMat4;

        fishModels.emplace_back(model);
        fishMaterialIds.emplace_back(0);
    }

    auto const device = LogicalDevice::GetVkDevice();
    auto const physicalDevice = LogicalDevice::GetPhysicalDevice();

    size_t const stateBufferSize = sizeof(FishState) * _fishInstanceMetadata.instanceCount;
    size_t const modelsBufferSize = sizeof(glm::mat4) * _fishInstanceMetadata.instanceCount;
    size_t const materialsBufferSize = sizeof(int32_t) * _fishInstanceMetadata.instanceCount;

    if (_fishStateBuffer == nullptr || _fishStateBuffer->bufferSize < stateBufferSize)
    {
        _fishStateBuffer = RB::CreateLocalStorageBuffer(device, physicalDevice, stateBufferSize, 1);
    }
    if (_fishModelsBuffer == nullptr || _fishModelsBuffer->bufferSize < modelsBufferSize)
    {
        _fishModelsBuffer = RB::CreateLocalStorageBuffer(
            device,
            physicalDevice,
            modelsBufferSize,
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        );
    }
    if (_fishMaterialsBuffer == nullptr || _fishMaterialsBuffer->bufferSize < materialsBufferSize)
    {
        _fishMaterialsBuffer = RB::CreateVertexBufferGroup(
            device,
            physicalDevice,
            materialsBufferSize,
            1
        );
    }
    
    auto const scheduleLocalUpload = [](std::shared_ptr<LocalBufferTracker> const & bufferTracker) -> void
    {
        auto remLifeTime =
            std::make_shared<int>(bufferTracker->LocalBuffer().buffers.size() + LogicalDevice::GetMaxFramePerFlight());
        LogicalDevice::AddRenderTask(
            [bufferTracker, remLifeTime](RT::CommandRecordState & recordState) -> bool
            {
                bufferTracker->Update(recordState);
                (*remLifeTime)--;
                return *remLifeTime > 0;
            }
        );
    };

    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _fishStateBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _fishStateBuffer->bufferSize, 1),
        Alias(boids.data(), boids.size())
    ));

    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _fishModelsBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _fishModelsBuffer->bufferSize, 1),
        Alias(fishModels.data(), fishModels.size())
    ));

    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _fishMaterialsBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _fishMaterialsBuffer->bufferSize, 1),
        Alias(fishMaterialIds.data(), fishMaterialIds.size())
    ));
}

//======================================================================================================================

std::vector<CollisionTriangle> BoidsSimulationApp::ExtractCollisionTriangles(uint32_t const vertexCount,
                                                                             uint32_t const indexCount,
                                                                             AS::GLTF::Vertex *vertices,
                                                                             AS::GLTF::Index *indices)
{
    std::vector<CollisionTriangle> triangles{};

    if (vertexCount == 0 || indexCount == 0 || vertices == nullptr || indices == nullptr)
    {
        return triangles;
    }

    MFA_ASSERT(indexCount % 3 == 0);
    triangles.reserve(indexCount / 3);

    for (uint32_t index = 0; index < indexCount; index += 3)
    {
        auto const i0 = indices[index + 0];
        auto const i1 = indices[index + 1];
        auto const i2 = indices[index + 2];

        MFA_ASSERT(i0 < vertexCount);
        MFA_ASSERT(i1 < vertexCount);
        MFA_ASSERT(i2 < vertexCount);

        auto const &v0 = vertices[i0].position;
        auto const &v1 = vertices[i1].position;
        auto const &v2 = vertices[i2].position;

        triangles.emplace_back(
            CollisionTriangle{.v0 = v0, .v1 = v1, .v2 = v2, .normal = glm::normalize(glm::cross(v1 - v0, v2 - v1))});
    }

    return triangles;
}

//======================================================================================================================

std::vector<CollisionTriangle>
BoidsSimulationApp::BakeCollisionTriangles(std::vector<CollisionTriangle> const &triangles,
                                           glm::mat4 const &modelMatrix)
{
    std::vector<CollisionTriangle> bakedTriangles{};
    bakedTriangles.reserve(triangles.size());

    for (auto const &triangle : triangles)
    {
        auto const v0 = glm::vec3(modelMatrix * glm::vec4(triangle.v0, 1.0f));
        auto const v1 = glm::vec3(modelMatrix * glm::vec4(triangle.v1, 1.0f));
        auto const v2 = glm::vec3(modelMatrix * glm::vec4(triangle.v2, 1.0f));

        bakedTriangles.emplace_back(
            CollisionTriangle{.v0 = v0, .v1 = v1, .v2 = v2, .normal = glm::normalize(glm::cross(v1 - v0, v2 - v0))});
    }

    return bakedTriangles;
}

//======================================================================================================================

void BoidsSimulationApp::PrepareScene()
{
    auto const fishModelPath = Path::Get("boidapp/fish/scene.gltf");
    auto const cubeModelPath = Path::Get("boidapp/cube/scene.gltf");
    auto const torusModelPath = Path::Get("boidapp/torus/torus.gltf");

    MFA_ASSERT(std::filesystem::exists(fishModelPath));
    MFA_ASSERT(std::filesystem::exists(cubeModelPath));
    MFA_ASSERT(std::filesystem::exists(torusModelPath));
    MFA_ASSERT(JobSystem::HasInstance() == true);

    auto const loadModel = [](std::string const &modelPath) -> std::shared_ptr<Importer::Model>
    {
        auto model = Importer::GLTF_Model(modelPath);
        MFA_ASSERT(model != nullptr);
        MFA_ASSERT(model->mesh != nullptr);

        if (model->mesh->IsOptimized() == false)
        {
            model->mesh->Optimize();
        }
        if (model->mesh->IsCentered() == false)
        {
            model->mesh->CenterMesh();
        }

        return model;
    };

    auto fishFuture = JobSystem::AssignTask<std::shared_ptr<Importer::Model>>(
        [fishModelPath, loadModel]() -> std::shared_ptr<Importer::Model> { return loadModel(fishModelPath); });
    auto cubeFuture = JobSystem::AssignTask<std::shared_ptr<Importer::Model>>(
        [cubeModelPath, loadModel]() -> std::shared_ptr<Importer::Model> { return loadModel(cubeModelPath); });
    auto torusFuture = JobSystem::AssignTask<std::shared_ptr<Importer::Model>>(
        [torusModelPath, loadModel]() -> std::shared_ptr<Importer::Model> { return loadModel(torusModelPath); });

    auto const fishGltfModel = fishFuture.get();
    auto const cubeGltfModel = cubeFuture.get();
    auto const torusGltfModel = torusFuture.get();

    MFA_ASSERT(fishGltfModel != nullptr);
    MFA_ASSERT(cubeGltfModel != nullptr);
    MFA_ASSERT(torusGltfModel != nullptr);

    std::vector<BlinnPhongPipeline::Vertex> sceneVertices{};
    std::vector<AS::GLTF::Index> sceneIndices{};
    std::vector<glm::mat4> sceneModels{};
    std::vector<int32_t> sceneMaterialIds{};
    std::vector<BlinnPhongPipeline::Material> materials{};
    std::vector<CollisionTriangle> collisionTriangles{};

    auto const appendMesh = [&sceneVertices, &sceneIndices](std::shared_ptr<Importer::Model> const &model,
                                                            BufferMetadata &metadata) -> void
    {
        auto const &mesh = model->mesh;
        auto const vertexCount = mesh->GetVertexCount();
        auto const indexCount = mesh->GetIndexCount();
        auto *vertices = mesh->GetVertexData()->As<AS::GLTF::Vertex>();
        auto *indices = mesh->GetIndexData()->As<AS::GLTF::Index>();

        auto const vertexOffset = sceneVertices.size();
        metadata.indexOffset = static_cast<uint32_t>(sceneIndices.size());
        metadata.indexCount = indexCount;

        sceneVertices.reserve(sceneVertices.size() + vertexCount);
        sceneIndices.reserve(sceneIndices.size() + indexCount);

        for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            sceneVertices.emplace_back(BlinnPhongPipeline::Vertex{.position = vertices[vertexIndex].position,
                                                                  .normal = vertices[vertexIndex].normal,
                                                                  .albedoUV = vertices[vertexIndex].baseColorUV});
        }

        for (uint32_t index = 0; index < indexCount; ++index)
        {
            sceneIndices.emplace_back(vertexOffset + indices[index]);
        }
    };

    appendMesh(fishGltfModel, _fishMeshMetadata);
    appendMesh(cubeGltfModel, _cageMeshMetadata);
    appendMesh(torusGltfModel, _torusMeshMetadata);

    glm::mat4 cageInstance{};
    {
        auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {10.0f, 10.0f, 10.0f});
        auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
        cageInstance = rotationMat * scaleMat;
    }

    std::vector<glm::mat4> torusInstances{};
    {
        {
            auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {5.0f, 5.0f, 5.0f});
            auto const rotationMat = Rotation{{0.0f, 0.0f, 90.0f}}.GetMatrix();
            torusInstances.emplace_back(rotationMat * scaleMat);
        }
        {
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {9.0f, 0.0f, 0.0f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {7.8f, 0.0f, 7.8f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {3.0f, 0.0f, 0.0f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {-9.0f, 0.0f, 0.0f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {-7.8f, 0.0f, -7.8f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {-3.0f, 0.0f, 0.0f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
        }
        {
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {9.0f, -5.0f, 0.0f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {7.8f, -5.0f, 7.8f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {3.0f, -5.0f, 0.0f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {-9.0f, -5.0f, 0.0f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {-7.8f, -5.0f, -7.8f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
            {
                auto const scaleMat = glm::scale(glm::identity<glm::mat4>(), {2.0f, 2.0f, 2.0f});
                auto const rotationMat = Rotation{{0.0f, 0.0f, 0.0f}}.GetMatrix();
                auto const translationMat = glm::translate(glm::identity<glm::mat4>(), {-3.0f, -5.0f, 0.0f});
                torusInstances.emplace_back(translationMat * rotationMat * scaleMat);
            }
        }
    }

    // Fish
    materials.emplace_back(BlinnPhongPipeline::Material{
        .albedo = glm::vec4(1.0f),
        .specularStrength = 0.0f,
        .shininess = 32.0f,
        .albedoTexture = 0
    });
    // Cage
    materials.emplace_back(BlinnPhongPipeline::Material{
        .albedo = glm::vec4(1.0f),
        .specularStrength = 0.0f,
        .shininess = 16.0f,
        .albedoTexture = 1,
        .enableLighting = 0
    });
    // Torus
    materials.emplace_back(BlinnPhongPipeline::Material{
        .albedo = glm::vec4(0.90f, 0.0f, 0.0f, 1.0f),
        .specularStrength = 0.0f,
        .shininess = 32.0f,
        .albedoTexture = -1
    });

    _cageInstanceMetadata.instanceOffset = static_cast<uint32_t>(sceneModels.size());
    _cageInstanceMetadata.instanceCount = 1;
    sceneModels.emplace_back(cageInstance);
    sceneMaterialIds.emplace_back(1);

    _torusInstanceMetadata.instanceOffset = static_cast<uint32_t>(sceneModels.size());
    _torusInstanceMetadata.instanceCount = static_cast<uint32_t>(torusInstances.size());
    for (auto const &torusInstance : torusInstances)
    {
        sceneModels.emplace_back(torusInstance);
        sceneMaterialIds.emplace_back(2);
    }

    auto const cubeTriangles =
        ExtractCollisionTriangles(cubeGltfModel->mesh->GetVertexCount(), cubeGltfModel->mesh->GetIndexCount(),
                                  cubeGltfModel->mesh->GetVertexData()->As<AS::GLTF::Vertex>(),
                                  cubeGltfModel->mesh->GetIndexData()->As<AS::GLTF::Index>());
    auto cageTriangles = BakeCollisionTriangles(cubeTriangles, cageInstance);
    for (auto &triangle : cageTriangles)
    {
        triangle.normal *= -1.0f;
    }
    collisionTriangles.insert(collisionTriangles.end(), cageTriangles.begin(), cageTriangles.end());

    // auto const torusTriangles =
    //     ExtractCollisionTriangles(torusGltfModel->mesh->GetVertexCount(), torusGltfModel->mesh->GetIndexCount(),
    //                               torusGltfModel->mesh->GetVertexData()->As<AS::GLTF::Vertex>(),
    //                               torusGltfModel->mesh->GetIndexData()->As<AS::GLTF::Index>());

    // for (auto const &torusInstance : torusInstances)
    // {
    //     auto bakedTorusTriangles = BakeCollisionTriangles(torusTriangles, torusInstance);
    //     collisionTriangles.insert(collisionTriangles.end(), bakedTorusTriangles.begin(), bakedTorusTriangles.end());
    // }

    MFA_ASSERT(sceneVertices.empty() == false);
    MFA_ASSERT(sceneIndices.empty() == false);
    MFA_ASSERT(sceneModels.empty() == false);
    MFA_ASSERT(sceneMaterialIds.empty() == false);
    MFA_ASSERT(materials.empty() == false);
    MFA_ASSERT(collisionTriangles.empty() == false);

    auto const device = LogicalDevice::GetVkDevice();
    auto const physicalDevice = LogicalDevice::GetPhysicalDevice();

    MFA_ASSERT(fishGltfModel->textures.empty() == false);
    MFA_ASSERT(cubeGltfModel->textures.empty() == false);

    {
        auto commandBuffer = RB::BeginSingleTimeCommand(
            device,
            *LogicalDevice::GetGraphicCommandPool()
        );

        std::array<uint8_t, 1> const mipLevels{0};
        auto [fishTexture, fishStageBuffer] = RB::CreateTexture(
            *fishGltfModel->textures[0],
            device,
            physicalDevice,
            commandBuffer,
            static_cast<int>(mipLevels.size()),
            const_cast<uint8_t *>(mipLevels.data())
        );
        auto [cageTexture, cageStageBuffer] = RB::CreateTexture(
            *cubeGltfModel->textures[0],
            device,
            physicalDevice,
            commandBuffer,
            static_cast<int>(mipLevels.size()),
            const_cast<uint8_t *>(mipLevels.data())
        );

        RB::EndAndSubmitSingleTimeCommand(
            device,
            *LogicalDevice::GetGraphicCommandPool(),
            LogicalDevice::GetGraphicQueue(),
            commandBuffer
        );

        _materialTextures[0] = std::move(fishTexture);
        _materialTextures[1] = std::move(cageTexture);
    }

    _sceneVertexBuffer = RB::CreateVertexBufferGroup(device, physicalDevice,
                                                     sizeof(BlinnPhongPipeline::Vertex) * sceneVertices.size(), 1);

    _sceneModelsBuffer = RB::CreateVertexBufferGroup(device, physicalDevice,
                                                     sizeof(glm::mat4) * sceneModels.size(), 1);
    _sceneMaterialsBuffer = RB::CreateVertexBufferGroup(
        device,
        physicalDevice,
        sizeof(int32_t) * sceneMaterialIds.size(),
        1
    );
    _materialBuffer = RB::CreateLocalStorageBuffer(
        device,
        physicalDevice,
        sizeof(BlinnPhongPipeline::Material) * materials.size(),
        1
    );
    {
        auto sceneIndexBuffer = RB::CreateBufferGroup(
            device, physicalDevice, sizeof(AS::GLTF::Index) * sceneIndices.size(), 1,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _sceneIndexBuffer = std::shared_ptr<RT::BufferGroup>(std::move(sceneIndexBuffer));
    }
    _sceneCollisionTriangleBuffer =
        RB::CreateLocalStorageBuffer(device, physicalDevice, sizeof(CollisionTriangle) * collisionTriangles.size(), 1);

    auto const scheduleLocalUpload = [](std::shared_ptr<LocalBufferTracker> const &bufferTracker) -> void
    {
        auto remLifeTime =
            std::make_shared<int>(bufferTracker->LocalBuffer().buffers.size() + LogicalDevice::GetMaxFramePerFlight());
        LogicalDevice::AddRenderTask(
            [bufferTracker, remLifeTime](RT::CommandRecordState &recordState) -> bool
            {
                bufferTracker->Update(recordState);
                (*remLifeTime)--;
                if (*remLifeTime <= 0)
                {
                    return false;
                }

                return true;
            });
    };

    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _sceneVertexBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _sceneVertexBuffer->bufferSize, 1),
        Alias(sceneVertices.data(), sceneVertices.size())));

    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _sceneModelsBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _sceneModelsBuffer->bufferSize, 1),
        Alias(sceneModels.data(), sceneModels.size())));

    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _sceneMaterialsBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _sceneMaterialsBuffer->bufferSize, 1),
        Alias(sceneMaterialIds.data(), sceneMaterialIds.size())));

    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _materialBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _materialBuffer->bufferSize, 1),
        Alias(materials.data(), materials.size())));

    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _sceneIndexBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _sceneIndexBuffer->bufferSize, 1),
        Alias(sceneIndices.data(), sceneIndices.size())));
        
    scheduleLocalUpload(std::make_shared<LocalBufferTracker>(
        _sceneCollisionTriangleBuffer,
        RB::CreateStageBuffer(device, physicalDevice, _sceneCollisionTriangleBuffer->bufferSize, 1),
        Alias(collisionTriangles.data(), collisionTriangles.size())));
}

//======================================================================================================================

void BoidsSimulationApp::PreparePipelines()
{
    _pUpdateFishCompute = std::make_unique<BoidsUpdateFishPipeline>();
    _pShadingGraphic = std::make_unique<BlinnPhongPipeline>(
        _sceneRenderPass->GetRenderPass(),
        BlinnPhongPipeline::Params{
            .dynamicCullMode = true,
            .modelOffset = 0,
            .modelBinding = 1,
            .materialOffset = 0,
            .materialBinding = 2,
            .bindingDescriptions = std::vector<VkVertexInputBindingDescription>{
                VkVertexInputBindingDescription{
                    .binding = 0,
                    .stride = sizeof(BlinnPhongPipeline::Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
                },
                VkVertexInputBindingDescription{
                    .binding = 1,
                    .stride = sizeof(glm::mat4),
                    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
                },
                VkVertexInputBindingDescription{
                    .binding = 2,
                    .stride = sizeof(int32_t),
                    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
                },
            },
        }
    );
}

//======================================================================================================================

void BoidsSimulationApp::PrepareRenderDescriptorSets()
{
    MFA_ASSERT(_sceneCollisionTriangleBuffer != nullptr);
    _dsColliders = _pUpdateFishCompute->CreateCollisionTrianglesDescriptorSets(*_sceneCollisionTriangleBuffer);

    MFA_ASSERT(_simulationConstantsBufferTracker != nullptr);
    _dsConstants = _pUpdateFishCompute->CreateSimulationConstantsDescriptorSets(
        _simulationConstantsBufferTracker->LocalBuffer()
    );

    MFA_ASSERT(_cameraBufferTracker != nullptr);
    _dsCamera = _pShadingGraphic->CreateCameraDescriptorSets(_cameraBufferTracker->HostVisibleBuffer());

    MFA_ASSERT(_lightBufferTracker != nullptr);
    _dsLighting = _pShadingGraphic->CreateLightBufferDescriptorSets(_lightBufferTracker->LocalBuffer());

    MFA_ASSERT(_materialBuffer != nullptr);
    MFA_ASSERT(_materialTextures[0] != nullptr);
    MFA_ASSERT(_materialTextures[1] != nullptr);
    std::array<RT::GpuTexture, 2> materialTextures{
        RT::GpuTexture(_materialTextures[0]->imageGroup, _materialTextures[0]->imageView),
        RT::GpuTexture(_materialTextures[1]->imageGroup, _materialTextures[1]->imageView)
    };
    _dsMaterial = _pShadingGraphic->CreateMaterialDescriptorSets(
        *_materialBuffer,
        *_sampler,
        materialTextures.size(),
        materialTextures.data()
    );

}

//======================================================================================================================

void BoidsSimulationApp::PrepareComputeDescriptorSets()
{
    MFA_ASSERT(_fishModelsBuffer != nullptr);
    MFA_ASSERT(_fishStateBuffer != nullptr);
    if (_dsUpdateFish.has_value() == false)
    {
        _dsUpdateFish = _pUpdateFishCompute->CreateFishDescriptorSets(*_fishStateBuffer, *_fishModelsBuffer);
    }
    else 
    {
        _pUpdateFishCompute->UpdateFishDescriptorSets(*_dsUpdateFish, *_fishStateBuffer, *_fishModelsBuffer);
    }
}

//======================================================================================================================

void BoidsSimulationApp::UpdateCamera(float deltaTime)
{
    _camera->Update(deltaTime);
    if (_camera->IsDirty())
    {
        MFA::BlinnPhongPipeline::Camera cameraData{.viewProjection = _camera->ViewProjection(),
                                                   .position = _camera->GlobalPosition()};
        _cameraBufferTracker->SetData(Alias{cameraData});
    }
}

//======================================================================================================================

void BoidsSimulationApp::UpdateBufferTrackers(RT::CommandRecordState const &recordState)
{
    _cameraBufferTracker->Update(recordState);
    _simulationConstantsBufferTracker->Update(recordState);
    _lightBufferTracker->Update(recordState);
}

//======================================================================================================================

void BoidsSimulationApp::Run()
{
    SDL_GL_SetSwapInterval(0);
    SDL_Event e;

    _time = Time::Instantiate(120, 30);

    bool shouldQuit = false;

    while (shouldQuit == false)
    {
        // Handle events
        while (SDL_PollEvent(&e) != 0)
        {
            // User requests quit
            if (e.type == SDL_QUIT)
            {
                shouldQuit = true;
            }
        }

        LogicalDevice::Update();

        Update(Time::DeltaTimeSec());

        auto recordState = LogicalDevice::AcquireRecordState(_swapChainResource->GetSwapChainImages().swapChain);
        if (recordState.isValid == true)
        {
            _activeImageIndex = static_cast<int>(recordState.imageIndex);
            Render(recordState);
            LogicalDevice::SubmitQueues(recordState);
            LogicalDevice::Present(recordState, _swapChainResource->GetSwapChainImages().swapChain);
        }

        _time->Update();
    }

    _time.reset();

    LogicalDevice::DeviceWaitIdle();
}

//======================================================================================================================

void BoidsSimulationApp::Update(float deltaTime)
{
    if (_sceneWindowResized == true)
    {
        PrepareSceneRenderPass();
        _sceneWindowResized = false;
        return;
    }

    // TODO: Whenever the rest button is pressed we have to respawn the fish in case the fish count has changed. We need
    // another parameter for it.

    UpdateCamera(deltaTime);

    _ui->Update();
}

//======================================================================================================================
void BoidsSimulationApp::Render(MFA::RT::CommandRecordState &recordState)
{
    LogicalDevice::BeginCommandBuffer(recordState, RT::CommandBufferType::Compute);

    UpdateBufferTrackers(recordState);

    _pUpdateFishCompute->BindPipeline(recordState);
    _pUpdateFishCompute->BindFishes(recordState, *_dsUpdateFish);
    _pUpdateFishCompute->BindCollisionTriangles(recordState, _dsColliders);
    _pUpdateFishCompute->BindSimulationConstants(recordState, _dsConstants);

    float fixedDT = 1.0f / 120.0f;
    _pUpdateFishCompute->SetPushConstants(
        recordState,
        BoidsUpdateFishPipeline::PushConstants {
            .dt = fixedDT,
            .fixedDt = fixedDT,
            .stateMask = 0xFFFFFF
        }
    );

    vkCmdDispatch(
        recordState.commandBuffer,
        (_fishInstanceMetadata.instanceCount + 255) / 256,
        1,
        1
    );

    LogicalDevice::EndCommandBuffer(recordState);

    LogicalDevice::BeginCommandBuffer(recordState, RT::CommandBufferType::Graphic);

    _sceneRenderPass->Begin(recordState, *_sceneFrameBuffer);

    _pShadingGraphic->BindPipeline(recordState);
    _pShadingGraphic->BindCameraDescriptorSet(recordState, _dsCamera);
    _pShadingGraphic->BindLightDescriptorSet(recordState, _dsLighting);
    _pShadingGraphic->BindMaterialDescriptorSet(recordState, _dsMaterial);

    RB::SetFrontFace(recordState.commandBuffer, VK_FRONT_FACE_CLOCKWISE);
    RB::SetCullMode(recordState.commandBuffer, VK_CULL_MODE_BACK_BIT); 

    RB::BindVertexBuffer(recordState, *_sceneVertexBuffer->buffers[0], 0, 0);
    RB::BindIndexBuffer(recordState, *_sceneIndexBuffer->buffers[0], 0);    

    // Fish
    RB::BindVertexBuffer(
        recordState, 
        *_fishModelsBuffer->buffers[recordState.frameIndex % _fishModelsBuffer->buffers.size()], 
        1, 
        0
    );
    RB::BindVertexBuffer(
        recordState,
        *_fishMaterialsBuffer->buffers[0],
        2,
        0
    );
    RB::DrawIndexed(
        recordState, 
        _fishMeshMetadata.indexCount, 
        _fishInstanceMetadata.instanceCount, 
        _fishMeshMetadata.indexOffset, 
        0,
        _fishInstanceMetadata.instanceOffset
    );

    // Static scene objects
    RB::BindVertexBuffer(recordState, *_sceneModelsBuffer->buffers[0], 1, 0);
    RB::BindVertexBuffer(recordState, *_sceneMaterialsBuffer->buffers[0], 2, 0);

    // Torus
    RB::DrawIndexed(
        recordState, 
        _torusMeshMetadata.indexCount, 
        _torusInstanceMetadata.instanceCount, 
        _torusMeshMetadata.indexOffset, 
        0,
        _torusInstanceMetadata.instanceOffset
    );

    RB::SetCullMode(recordState.commandBuffer, VK_CULL_MODE_FRONT_BIT); 

    // Cage
    RB::DrawIndexed(
        recordState, 
        _cageMeshMetadata.indexCount, 
        _cageInstanceMetadata.instanceCount, 
        _cageMeshMetadata.indexOffset, 
        0, 
        _cageInstanceMetadata.instanceOffset
    );

    _sceneRenderPass->End(recordState);

    _displayRenderPass->Begin(recordState);

    _ui->Render(recordState, Time::DeltaTimeSec());

    _displayRenderPass->End(recordState);

    LogicalDevice::EndCommandBuffer(recordState);
}

//======================================================================================================================

void BoidsSimulationApp::Resize() 
{ 
    _camera->SetProjectionDirty();
    _sceneWindowResized = true; 
}

//======================================================================================================================

void BoidsSimulationApp::Reload() { LogicalDevice::DeviceWaitIdle(); }

//======================================================================================================================

void BoidsSimulationApp::OnSDL_Event(SDL_Event *event)
{
    // if (UI::Instance != nullptr && UI::Instance->HasFocus() == true)
    // {
    //     return;
    // }

    // Keyboard
    if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.sym == SDLK_F5)
        {
            Reload();
        }
    }

    { // Joystick
    }
}

//======================================================================================================================

void BoidsSimulationApp::OnUI(float deltaTimeSec)
{
    ApplyUI_Style();
    _ui->DisplayDockSpace();
    DisplaySimulationParameterWindow();
    DisplayLightingParametersWindow();
    DisplaySceneWindow();
}

//======================================================================================================================

void BoidsSimulationApp::PrepareSceneRenderPass()
{
    auto const maxImageCount = LogicalDevice::GetSwapChainImageCount();

    if (_sceneRenderResource != nullptr || _sceneRenderPass != nullptr || _sceneFrameBuffer != nullptr)
    {
        MFA_ASSERT(_sceneRenderResource != nullptr);
        MFA_ASSERT(_sceneRenderPass != nullptr);
        MFA_ASSERT(_sceneFrameBuffer != nullptr);

        struct OldScene
        {
            std::shared_ptr<SceneRenderResource> sceneRenderResource{};
            std::shared_ptr<SceneFrameBuffer> sceneFrameBuffer{};
            std::vector<ImTextureID> textureIDs{};
            int remLifeTime{};
        };
        auto oldScene = std::make_shared<OldScene>();
        oldScene->sceneRenderResource = _sceneRenderResource;
        oldScene->sceneFrameBuffer = _sceneFrameBuffer;
        oldScene->textureIDs = _sceneTextureID_List;
        oldScene->remLifeTime = LogicalDevice::GetMaxFramePerFlight() * 2 + 1;
        LogicalDevice::AddRenderTask(
            [oldScene](RT::CommandRecordState &recordState) -> bool
            {
                oldScene->remLifeTime--;
                if (oldScene->remLifeTime <= 0)
                {
                    return false;
                }
                return true;
            });

        _sceneTextureID_List.clear();
    }

    auto const surfaceFormat = LogicalDevice::GetSurfaceFormat().format;
    auto const depthFormat = LogicalDevice::GetDepthFormat();
    auto const sampleCount = LogicalDevice::GetMaxSampleCount();

    _sceneRenderResource =
        std::make_shared<SceneRenderResource>(_sceneWindowSize, surfaceFormat, depthFormat, sampleCount);
    if (_sceneRenderPass == nullptr)
    {
        _sceneRenderPass = std::make_unique<SceneRenderPass>(surfaceFormat, depthFormat, sampleCount);
    }
    _sceneFrameBuffer = std::make_shared<SceneFrameBuffer>(_sceneRenderResource, _sceneRenderPass->GetRenderPass());
    _sceneTextureID_List.resize(maxImageCount);

    for (int imageIndex = 0; imageIndex < maxImageCount; imageIndex++)
    {
        auto &sceneTextureID = _sceneTextureID_List[imageIndex];
        sceneTextureID =
            _ui->AddTexture(_sampler->sampler, _sceneRenderResource->ColorImage(imageIndex).imageView->imageView);
    }
}

//======================================================================================================================

void BoidsSimulationApp::ApplyUI_Style()
{
    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;

    // Backgrounds
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Headers
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.50f, 0.75f, 0.85f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.55f, 0.80f, 0.95f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.60f, 0.85f, 1.00f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.45f, 0.70f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.50f, 0.75f, 0.90f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.55f, 0.80f, 1.00f);

    // Text and Highlights
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // Bright white text
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f); // Muted gray for disabled
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.30f, 0.60f, 0.90f, 0.90f); // Text selection

    // Frame Backgrounds
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.50f, 0.80f, 0.70f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.55f, 0.85f, 0.85f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.55f, 0.80f, 0.90f);
    colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.50f, 0.75f, 1.00f);

    // Title Bars
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.55f, 0.80f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.07f, 0.50f);

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.50f, 0.75f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.55f, 0.80f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.60f, 0.85f, 1.00f);

    // Style Tweaks
    style.FrameRounding = 5.0f;
    style.WindowRounding = 5.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 5.0f;
    style.ChildRounding = 5.0f;
    style.TabRounding = 4.0f;
}

//======================================================================================================================

void BoidsSimulationApp::DisplaySimulationParameterWindow()
{
    _ui->BeginWindow("Simulation parameters");

    bool simulationConstantsChanged = false;
    auto checkboxInt = [&simulationConstantsChanged](char const *label, int &value) -> void
    {
        bool checked = value != 0;
        if (ImGui::Checkbox(label, &checked))
        {
            value = checked ? 1 : 0;
            simulationConstantsChanged = true;
        }
    };

    //--------------------------------------------------------------

    ImGui::Checkbox("Play", &_play);
    _step = ImGui::Button("Step");
    _reset = ImGui::Button("Reset");

    ImGui::InputInt("Fish x count", &_config.fishXCount);
    ImGui::InputInt("Fish y count", &_config.fishYCount);
    ImGui::InputInt("Fish z count", &_config.fishZCount);

    //--------------------------------------------------------------
    ImGui::Separator();
    //--------------------------------------------------------------

    checkboxInt("Enable separation force", _config.simulation.bEnableSeparationForce);
    checkboxInt("Enable alignment force", _config.simulation.bEnableAlignmentForce);
    checkboxInt("Enable cohesion force", _config.simulation.bEnableCohesionForce);
    checkboxInt("Enable soft collision handling", _config.simulation.bEnableSoftCollisionHandling);
    checkboxInt("Enable soft collision for boundary", _config.simulation.bEnableSoftCollisionForBoundary);
    checkboxInt("Enable hard collision handling", _config.simulation.bEnableHardCollisionHandling);
    checkboxInt("Enable boundary collision handling", _config.simulation.bEnableBoundaryCollisionHandling);

    //--------------------------------------------------------------
    ImGui::Separator();
    //--------------------------------------------------------------

    simulationConstantsChanged =
        ImGui::InputFloat("Separation radius", &_config.simulation.separationRadius) || simulationConstantsChanged;
    simulationConstantsChanged =
        ImGui::InputFloat("Separation cos", &_config.simulation.separationCos) || simulationConstantsChanged;
    simulationConstantsChanged =
        ImGui::InputFloat("Separation constant", &_config.simulation.separationConstant) || simulationConstantsChanged;

    //--------------------------------------------------------------
    ImGui::Separator();
    //--------------------------------------------------------------

    simulationConstantsChanged =
        ImGui::InputFloat("Alignment radius", &_config.simulation.alignmentRadius) || simulationConstantsChanged;
    simulationConstantsChanged =
        ImGui::InputFloat("Alignment cos", &_config.simulation.alignmentCos) || simulationConstantsChanged;
    simulationConstantsChanged =
        ImGui::InputFloat("Alignment constant", &_config.simulation.alignmentConstant) || simulationConstantsChanged;


    //--------------------------------------------------------------
    ImGui::Separator();
    //--------------------------------------------------------------

    simulationConstantsChanged =
        ImGui::InputFloat("Cohesion radius", &_config.simulation.cohesionRadius) || simulationConstantsChanged;
    simulationConstantsChanged =
        ImGui::InputFloat("Cohesion cos", &_config.simulation.cohesionCos) || simulationConstantsChanged;
    simulationConstantsChanged =
        ImGui::InputFloat("Cohesion constant", &_config.simulation.cohesionConstant) || simulationConstantsChanged;

    //--------------------------------------------------------------
    ImGui::Separator();
    //--------------------------------------------------------------

    checkboxInt("Clamp speed", _config.simulation.bClampSpeed);
    simulationConstantsChanged =
        ImGui::InputFloat("Min speed", &_config.simulation.minSpeed) || simulationConstantsChanged;
    simulationConstantsChanged =
        ImGui::InputFloat("Max speed", &_config.simulation.maxSpeed) || simulationConstantsChanged;
    checkboxInt("Clamp acceleration", _config.simulation.bClampAcc);
    simulationConstantsChanged =
        ImGui::InputFloat("Min acceleration", &_config.simulation.minAcc) || simulationConstantsChanged;
    simulationConstantsChanged =
        ImGui::InputFloat("Max acceleration", &_config.simulation.maxAcc) || simulationConstantsChanged;

    //--------------------------------------------------------------
    ImGui::Separator();
    //--------------------------------------------------------------

    simulationConstantsChanged = ImGui::InputFloat("Soft collision offset", &_config.simulation.softCollisionOffset) ||
        simulationConstantsChanged;
    simulationConstantsChanged = ImGui::InputFloat("Hard collision offset", &_config.simulation.hardCollisionOffset) ||
        simulationConstantsChanged;

    //--------------------------------------------------------------
    ImGui::Separator();
    //--------------------------------------------------------------

    // ImGui::Checkbox("Enable submarine separation force", &_config.simulation.bEnableSubMarineSeparationForce);
    // ImGui::Checkbox("Enable submarine alignment force", &_config.simulation.bEnableSubMarineAlignmentForce);
    // ImGui::Checkbox("Enable submarine cohesion force", &_config.simulation.bEnableSubMarineCohesionForce);

    // ImGui::InputFloat("Submarine speed", &_config.simulation.subMarineSpeed);
    // ImGui::InputFloat("Submarine separation radius", &_config.simulation.subMarineSeparationRadius);
    // ImGui::InputFloat("Submarine alignment radius", &_config.simulation.subMarineAlignmentRadius);
    // ImGui::InputFloat("Submarine cohesion radius", &_config.simulation.subMarineCohesionRadius);

    // ImGui::InputFloat("Submarine separation euler angle", &_config.simulation.subMarineSeparationEulerAngle);
    // ImGui::InputFloat("Submarine alignment euler angle", &_config.simulation.subMarineAlignmentEulerAngle);
    // ImGui::InputFloat("Submarine cohesion euler angle", &_config.simulation.subMarineCohesionEulerAngle);

    // ImGui::InputFloat("Submarine separation cos", &_config.simulation.subMarineSeparationCos);
    // ImGui::InputFloat("Submarine alignment cos", &_config.simulation.subMarineAlignmentCos);
    // ImGui::InputFloat("Submarine cohesion cos", &_config.simulation.subMarineCohesionCos);

    // ImGui::InputFloat("Submarine separation constant", &_config.simulation.subMarineSeparationConstant);
    // ImGui::InputFloat("Submarine alignment constant", &_config.simulation.subMarineAlignmentConstant);
    // ImGui::InputFloat("Submarine cohesion constant", &_config.simulation.subMarineCohesionConstant);

    if (simulationConstantsChanged)
    {
        _simulationConstantsBufferTracker->SetData(Alias{_config.simulation});
    }

    _ui->EndWindow();
}

//======================================================================================================================

void BoidsSimulationApp::DisplayLightingParametersWindow()
{
    _ui->BeginWindow("Lighting parameters");

    bool lightChanged = false;

    lightChanged = ImGui::InputFloat3("Light direction", reinterpret_cast<float *>(&_light.direction)) || lightChanged;
    lightChanged = ImGui::ColorPicker4("Light color", reinterpret_cast<float *>(&_light.color)) || lightChanged;
    lightChanged = ImGui::SliderFloat("Light intensity", &_light.intensity, 0.0f, 10.0f) || lightChanged;
    lightChanged = ImGui::SliderFloat("Ambient intensity", &_light.ambientStrength, 0.0f, 1.0f) || lightChanged;
    lightChanged = ImGui::SliderFloat("Specularity", &_light.specularLightIntensity, 0.0f, 10.0f) || lightChanged;
    lightChanged = ImGui::InputInt("Shininess", &_light.shininess, 1, 256) || lightChanged;

    if (lightChanged && _lightBufferTracker != nullptr)
    {
        auto lightData = ToGpu(_light);
        _lightBufferTracker->SetData(Alias{lightData});
    }

    _ui->EndWindow();
}

//======================================================================================================================

void BoidsSimulationApp::DisplaySceneWindow()
{
    _ui->BeginWindow("Scene");
    auto sceneWindowSize = ImGui::GetWindowSize();
    _sceneWindowFocused = ImGui::IsWindowFocused() && ImGui::IsWindowDocked();
    sceneWindowSize.y -= 45.0f;
    sceneWindowSize.x -= 15.0f;
    if (_sceneWindowSize.width != sceneWindowSize.x || _sceneWindowSize.height != sceneWindowSize.y)
    {
        _sceneWindowSize.width = std::max(sceneWindowSize.x, 1.0f);
        _sceneWindowSize.height = std::max(sceneWindowSize.y, 1.0f);
        Resize();
    }
    if (_activeImageIndex < _sceneTextureID_List.size())
    {
        ImGui::Image(_sceneTextureID_List[_activeImageIndex], sceneWindowSize);
    }
    _ui->EndWindow();
}

//======================================================================================================================
