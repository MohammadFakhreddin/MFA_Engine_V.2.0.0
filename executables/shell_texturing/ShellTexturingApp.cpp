//
// Created by mohammad on 2026-03-08.
//

#include "ShellTexturingApp.hpp"

#include "BedrockAssert.hpp"
#include "BedrockDeffer.hpp"
#include "BedrockPath.hpp"
#include "ImportTexture.hpp"
#include "LogicalDevice.hpp"
#include "RenderBackend.hpp"
#include "ShapeGenerator.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

using namespace mfa;

//======================================================================================================================

ShellTexturingApp::ShellTexturingApp()
{
    if (SDL_JoystickOpen(0) != nullptr)
        SDL_JoystickEventState(SDL_ENABLE);

    LogicalDevice::SDL_EventSignal.Register([&](SDL_Event *event) -> void { OnSDL_Event(event); });

    _swapChainResource = std::make_shared<SwapChainRenderResource>();
    _depthResource = std::make_shared<DepthRenderResource>();
    _msaaResource = std::make_shared<MSSAA_RenderResource>();
    _displayRenderPass = std::make_shared<DisplayRenderPass>(_swapChainResource, _depthResource, _msaaResource);

    _sampler = RB::CreateSampler(LogicalDevice::GetVkDevice(), RB::CreateSamplerParams{.compareEnable = false});

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

    LogicalDevice::ResizeEventSignal2.Register([this]() -> void { Resize(); });

    PrepareSceneRenderPass();
    PrepareUniformBuffers();
    PrepareMesh();
    PreparePipeline();
    PrepareDescriptorSets();
}

//======================================================================================================================

ShellTexturingApp::~ShellTexturingApp() = default;

//======================================================================================================================

void ShellTexturingApp::Run()
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

void ShellTexturingApp::Update(float deltaTime)
{
    if (_sceneWindowResized == true)
    {
        PrepareSceneRenderPass();
        _sceneWindowResized = false;
        return;
    }

    UpdateCamera(deltaTime);
    _ui->Update();
}

//======================================================================================================================

void ShellTexturingApp::Render(mfa::RT::CommandRecordState &recordState)
{
    LogicalDevice::BeginCommandBuffer(recordState, RT::CommandBufferType::Graphic);

    UpdateBuffers(recordState);

    _sceneRenderPass->Begin(recordState, *_sceneFrameBuffer);

    _pipeline->BindPipeline(recordState);
    _pipeline->BindDescriptorSets(recordState, _cameraDescriptorSets, _shellDescriptorSets, _modelDescriptorSets,
                                  _materialDescriptorSets, _albedoTextureDescriptorSet, _lightDescriptorSets);

    auto &settings = _meshSettings[static_cast<int>(_selectedMesh)];

    RB::BindVertexBuffer(recordState, *_vertexBuffer->buffers.front(), 0, settings.vertexOffset);
    RB::BindIndexBuffer(recordState, *_indexBuffer->buffers.front(), settings.indexOffset);
    RB::DrawIndexed(recordState, settings.indexCount, static_cast<uint32_t>(settings.shell.count));

    _sceneRenderPass->End(recordState);

    _displayRenderPass->Begin(recordState);

    _ui->Render(recordState, Time::DeltaTimeSec());

    _displayRenderPass->End(recordState);

    LogicalDevice::EndCommandBuffer(recordState);
}

//======================================================================================================================

void ShellTexturingApp::Resize()
{
    for (auto &meshSetting : _meshSettings)
    {
        meshSetting.camera.SetProjectionDirty();
    }
    _sceneWindowResized = true;
}

//======================================================================================================================

void ShellTexturingApp::Reload()
{
    MFA_LOG_INFO("Reloading shell texturing app");
    LogicalDevice::DeviceWaitIdle();
    if (_pipeline != nullptr)
    {
        _pipeline->Reload();
    }
}

//======================================================================================================================

void ShellTexturingApp::OnSDL_Event(SDL_Event *event)
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

void ShellTexturingApp::OnUI(float deltaTimeSec)
{
    ApplyUI_Style();
    _ui->DisplayDockSpace();
    DisplayParametersWindow();
    DisplaySceneWindow();
}

//======================================================================================================================

void ShellTexturingApp::PrepareSceneRenderPass()
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
        oldScene->remLifeTime = LogicalDevice::GetMaxFramePerFlight() + 1;
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

void ShellTexturingApp::PrepareMesh()
{
    std::vector<ShellTexturingPipeline::Vertex> allVertices{};
    std::vector<uint32_t> allIndices{};

    ShapeGenerator::Vertices tempPositions{};
    ShapeGenerator::Indices tempIndices{};
    ShapeGenerator::Normals tempNormals{};
    ShapeGenerator::TextureCoordinates tempTextureCoordinates{};

    auto const clearTempMesh = [&]() -> void
    {
        tempPositions.clear();
        tempIndices.clear();
        tempNormals.clear();
        tempTextureCoordinates.clear();
    };

    auto const appendTempMesh = [&]() -> void
    {
        MFA_ASSERT(tempPositions.size() == tempNormals.size());
        MFA_ASSERT(tempPositions.size() == tempTextureCoordinates.size());

        allVertices.reserve(allVertices.size() + tempPositions.size());
        for (size_t vertexIndex = 0; vertexIndex < tempPositions.size(); ++vertexIndex)
        {
            allVertices.emplace_back(ShellTexturingPipeline::Vertex{
                .position = tempPositions[vertexIndex],
                .normal = tempNormals[vertexIndex],
                .uv = tempTextureCoordinates[vertexIndex],
            });
        }
        allIndices.insert(allIndices.end(), tempIndices.begin(), tempIndices.end());
    };

    {
        clearTempMesh();
        ShapeGenerator::PlaneNoAlloc(2.0f, 2.0f, 10, 10, tempPositions, tempIndices, tempNormals,
                                     tempTextureCoordinates);

        auto &settings = _meshSettings[static_cast<int>(SelectedMesh::Plane)];
        settings.vertexOffset = allVertices.size() * sizeof(ShellTexturingPipeline::Vertex);
        settings.indexOffset = allIndices.size() * sizeof(uint32_t);
        settings.indexCount = static_cast<uint32_t>(tempIndices.size());

        appendTempMesh();
    }
    {
        clearTempMesh();
        ShapeGenerator::SphereNoAlloc(1.0f, 4, tempPositions, tempIndices, tempNormals, tempTextureCoordinates);

        auto &settings = _meshSettings[static_cast<int>(SelectedMesh::Sphere)];
        settings.vertexOffset = allVertices.size() * sizeof(ShellTexturingPipeline::Vertex);
        settings.indexOffset = allIndices.size() * sizeof(uint32_t);
        settings.indexCount = static_cast<uint32_t>(tempIndices.size());

        appendTempMesh();
    }

    MFA_ASSERT(allVertices.empty() == false);
    MFA_ASSERT(allIndices.empty() == false);

    auto const device = LogicalDevice::GetVkDevice();
    auto const physicalDevice = LogicalDevice::GetPhysicalDevice();
    auto const vertexBufferSize = sizeof(ShellTexturingPipeline::Vertex) * allVertices.size();
    auto const indexBufferSize = sizeof(uint32_t) * allIndices.size();

    _vertexBuffer = RB::CreateVertexBufferGroup(device, physicalDevice, vertexBufferSize, 1);
    _indexBuffer = RB::CreateBufferGroup(device, physicalDevice, indexBufferSize, 1,
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _vertexBufferTracker = std::make_unique<LocalBufferTracker>(
        _vertexBuffer, RB::CreateStageBuffer(device, physicalDevice, vertexBufferSize, 1),
        Alias{allVertices.data(), allVertices.size()});
    _indexBufferTracker = std::make_unique<LocalBufferTracker>(
        _indexBuffer, RB::CreateStageBuffer(device, physicalDevice, indexBufferSize, 1),
        Alias{allIndices.data(), allIndices.size()});

    if (_albedoTexture == nullptr)
    {
        auto const cpuTexture = Importer::ErrorTexture();
        auto commandBuffer = RB::BeginSingleTimeCommand(device, *LogicalDevice::GetGraphicCommandPool());
        std::array<uint8_t, 1> mipLevels{0};
        auto [texture, stagingBuffer] = RB::CreateTexture(*cpuTexture, device, physicalDevice, commandBuffer,
                                                          static_cast<int>(mipLevels.size()), mipLevels.data());
        RB::EndAndSubmitSingleTimeCommand(device, *LogicalDevice::GetGraphicCommandPool(),
                                          LogicalDevice::GetGraphicQueue(), commandBuffer);
        (void)stagingBuffer;
        _albedoTexture = std::move(texture);
    }
}

//======================================================================================================================

void ShellTexturingApp::ApplySelectedMesh()
{
    int meshIndex = static_cast<int>(_selectedMesh);
    auto &settings = _meshSettings[meshIndex];
    _shellBufferTracker->SetData(Alias{settings.shell});
    _materialBufferTracker->SetData(Alias{settings.material});
    ShellTexturingPipeline::Camera const cameraData{.viewProjection = settings.camera.ViewProjection()};
    _cameraBufferTracker->SetData(Alias{cameraData});
    _modelBufferTracker->SetData(Alias{settings.model});
}

//======================================================================================================================

void ShellTexturingApp::PrepareUniformBuffers()
{
    auto const device = LogicalDevice::GetVkDevice();
    auto const physicalDevice = LogicalDevice::GetPhysicalDevice();
    auto const frameCount = LogicalDevice::GetMaxFramePerFlight();

    auto selectedMeshIndex = static_cast<int>(_selectedMesh);
    auto &settings = _meshSettings[selectedMeshIndex];

    _cameraBufferTracker = std::make_unique<HostVisibleBufferTracker>(
        RB::CreateHostVisibleUniformBuffer(device, physicalDevice, sizeof(ShellTexturingPipeline::Camera), frameCount),
        Alias{ShellTexturingPipeline::Camera{.viewProjection = settings.camera.ViewProjection()}});

    _shellBufferTracker = std::make_unique<HostVisibleBufferTracker>(
        RB::CreateHostVisibleUniformBuffer(device, physicalDevice, sizeof(ShellTexturingPipeline::Shell), frameCount),
        Alias{settings.shell});

    _modelBufferTracker = std::make_unique<HostVisibleBufferTracker>(
        RB::CreateHostVisibleUniformBuffer(device, physicalDevice, sizeof(ShellTexturingPipeline::Model), frameCount),
        Alias{settings.model});

    _materialBufferTracker = std::make_unique<HostVisibleBufferTracker>(
        RB::CreateHostVisibleUniformBuffer(device, physicalDevice, sizeof(ShellTexturingPipeline::Material),
                                           frameCount),
        Alias{settings.material});

    _lightBufferTracker = std::make_unique<HostVisibleBufferTracker>(
        RB::CreateHostVisibleUniformBuffer(device, physicalDevice, sizeof(_light), frameCount), Alias{_light});
}

//======================================================================================================================

void ShellTexturingApp::PreparePipeline()
{
    _pipeline = std::make_unique<ShellTexturingPipeline>(_sceneRenderPass->GetRenderPass());
}

//======================================================================================================================

void ShellTexturingApp::PrepareDescriptorSets()
{
    _cameraDescriptorSets = _pipeline->CreateCameraDescriptorSets(_cameraBufferTracker->HostVisibleBuffer());
    _shellDescriptorSets = _pipeline->CreateShellDescriptorSets(_shellBufferTracker->HostVisibleBuffer());
    _modelDescriptorSets = _pipeline->CreateModelDescriptorSets(_modelBufferTracker->HostVisibleBuffer());
    _materialDescriptorSets = _pipeline->CreateMaterialDescriptorSets(_materialBufferTracker->HostVisibleBuffer());
    _albedoTextureDescriptorSet = _pipeline->CreateAlbedoTextureDescriptorSet(*_sampler, *_albedoTexture);
    _lightDescriptorSets = _pipeline->CreateLightDescriptorSets(_lightBufferTracker->HostVisibleBuffer());
}

//======================================================================================================================

void ShellTexturingApp::UpdateCamera(float const deltaTime)
{
    auto &camera = _meshSettings[static_cast<int>(_selectedMesh)].camera;
    camera.Update(deltaTime);
    if (camera.IsDirty())
    {
        ShellTexturingPipeline::Camera const data{.viewProjection = camera.ViewProjection()};
        _cameraBufferTracker->SetData(Alias{data});
    }
}

//======================================================================================================================

void ShellTexturingApp::UpdateBuffers(RT::CommandRecordState const &recordState)
{
    _vertexBufferTracker->Update(recordState);
    _indexBufferTracker->Update(recordState);
    _cameraBufferTracker->Update(recordState);
    _shellBufferTracker->Update(recordState);
    _modelBufferTracker->Update(recordState);
    _materialBufferTracker->Update(recordState);
    _lightBufferTracker->Update(recordState);
}

//======================================================================================================================

void ShellTexturingApp::ApplyUI_Style()
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

void ShellTexturingApp::DisplayParametersWindow()
{
    _ui->BeginWindow("Parameters");
    MFA_DEFFER([this]() -> void { _ui->EndWindow(); });

    int selectedMesh = static_cast<int>(_selectedMesh);

    constexpr std::array meshNames{"Sphere", "Plane"};
    if (ImGui::Combo("Mesh", &selectedMesh, meshNames.data(), static_cast<int>(meshNames.size())))
    {
        _selectedMesh = static_cast<SelectedMesh>(selectedMesh);
        ApplySelectedMesh();
        return;
    }

    { // Shell settings
        auto &shell = _meshSettings[static_cast<int>(_selectedMesh)].shell;

        bool shellChanged = false;
        shellChanged |= ImGui::SliderFloat("Fur height", &shell.displacement, 0.001f, 0.8f, "%.3f");
        shellChanged |= ImGui::SliderFloat("Hair radius", &shell.radius, 0.01f, 10.0f, "%.3f");
        shellChanged |= ImGui::SliderInt("Shell count", &shell.count, 1, 512);
        shellChanged |= ImGui::SliderFloat("Shell height attenuation", &shell.attenuation, 0.1f, 5.0f, "%.2f");
        shellChanged |= ImGui::SliderInt("Shell density", &shell.density, 1, 1000);
        if (shellChanged)
        {
            _shellBufferTracker->SetData(Alias{shell});
        }
    }

    { // Material settings
        auto &material = _meshSettings[static_cast<int>(_selectedMesh)].material;
        if (ImGui::ColorEdit4("Albedo", &material.albedo.x))
        {
            _materialBufferTracker->SetData(Alias{material});
        }
    }

    {
        bool lightChanged = false;
        lightChanged |= ImGui::DragFloat3("Light direction", &_light.direction.x, 0.01f, -1.0f, 1.0f);
        lightChanged |= ImGui::ColorEdit3("Light color", &_light.color.x);
        lightChanged |= ImGui::SliderFloat("Ambient strength", &_light.ambientStrength, 0.0f, 1.0f);
        if (lightChanged)
        {
            _lightBufferTracker->SetData(Alias{_light});
        }
    }

    if (ImGui::CollapsingHeader("Camera"))
    {
        _meshSettings[static_cast<int>(_selectedMesh)].camera.Debug_UI();
    }
}

//======================================================================================================================

void ShellTexturingApp::DisplaySceneWindow()
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
