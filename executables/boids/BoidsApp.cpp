//
// Created by mohammad on 2026-03-08.
//

#include "BoidsApp.hpp"

#include "BedrockMath.hpp"
#include "BedrockPath.hpp"
#include "BlinnPhongPipeline.hpp"
#include "BoidsFish.hpp"
#include "LogicalDevice.hpp"

#include <filesystem>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>

using namespace MFA;

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
    PrepareFishStorageBuffer();
}

//======================================================================================================================

BoidsSimulationApp::~BoidsSimulationApp() = default;

//======================================================================================================================

void BoidsSimulationApp::PrepareUI()
{
    _ui = std::make_shared<UI>(
        _displayRenderPass,
        UI::Params{
            .lightMode = false,
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
            }
        }
    );
    _ui->UpdateSignal.Register([this]() -> void { OnUI(Time::DeltaTimeSec()); });
}

//======================================================================================================================

void BoidsSimulationApp::PrepareCamera()
{
    _camera = std::make_unique<MFA::ObserverCamera>(
        [this]()->VkExtent2D
        {
            return _sceneWindowSize;
        },
        [this]()->bool{return _sceneWindowFocused;}
    );

    _camera->SetLocalPosition(glm::vec3{ -54.0f, -36.0f, 69.0f });
    _camera->SetLocalRotation(Rotation{ {-23.0f, 140.0f, 180.0f} });
    _camera->Update(1.0f / 120.0f);

    auto cameraBuffer = RB::CreateHostVisibleUniformBuffer(
        LogicalDevice::GetVkDevice(),
        LogicalDevice::GetPhysicalDevice(),
        sizeof(BlinnPhongPipeline::Camera),
        LogicalDevice::GetMaxFramePerFlight()
    );
    _cameraBufferTracker = std::make_unique<HostVisibleBufferTracker>(cameraBuffer);

    BlinnPhongPipeline::Camera cameraData
    {
        .viewProjection = _camera->ViewProjection(),
        .position = _camera->GlobalPosition()
    };
    _cameraBufferTracker->SetData(Alias{cameraData});
}

//======================================================================================================================

void BoidsSimulationApp::PrepareFishStorageBuffer()
{
    auto const spawnBoids = [this]() -> std::vector<Fish>
    {
        std::vector<Fish> boids{};

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
                    Fish boid{};
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

                    auto const velX = Math::Random<float>(speedMin, speedMax);
                    auto const velY = Math::Random<float>(speedMin, speedMax);
                    auto const velZ = Math::Random<float>(speedMin, speedMax);
                    boid.rbVelocity = glm::vec3{
                        velX,
                        velY,
                        velZ
                    };

                    boids.emplace_back(boid);
                }
            }
        }

        return boids;
    };

    auto const boids = spawnBoids();

    if (_fishStorageBufferTracker == nullptr)
    {
        auto fishStorageBuffer = RB::CreateHostVisibleStorageBuffer(
            LogicalDevice::GetVkDevice(),
            LogicalDevice::GetPhysicalDevice(),
            sizeof(Fish) * boids.size(),
            LogicalDevice::GetMaxFramePerFlight()
        );

        _fishStorageBufferTracker = std::make_unique<HostVisibleBufferTracker>(
            fishStorageBuffer,
            Alias(boids.data(), boids.size())
        );
    }
    else 
    {
        _fishStorageBufferTracker->SetData(Alias(boids.data(), boids.size()));
    }
}

//======================================================================================================================

void BoidsSimulationApp::UpdateCamera(float deltaTime)
{
    _camera->Update(deltaTime);
    if (_camera->IsDirty())
    {
        MFA::BlinnPhongPipeline::Camera cameraData
        {
            .viewProjection = _camera->ViewProjection(),
            .position = _camera->GlobalPosition()
        };
        _cameraBufferTracker->SetData(Alias{cameraData});
    }
}

//======================================================================================================================

void BoidsSimulationApp::UpdateBufferTrackers(RT::CommandRecordState const & recordState)
{
    _cameraBufferTracker->Update(recordState);
    _fishStorageBufferTracker->Update(recordState);
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

    UpdateCamera(deltaTime);

    _ui->Update();
}

//======================================================================================================================

void BoidsSimulationApp::Render(MFA::RT::CommandRecordState &recordState)
{
    UpdateBufferTrackers(recordState);

    // LogicalDevice::BeginCommandBuffer(
    //     recordState,
    //     RT::CommandBufferType::Compute
    // );
    // LogicalDevice::EndCommandBuffer(recordState);

    LogicalDevice::BeginCommandBuffer(recordState, RT::CommandBufferType::Graphic);

    _sceneRenderPass->Begin(recordState, *_sceneFrameBuffer);

    _sceneRenderPass->End(recordState);

    _displayRenderPass->Begin(recordState);

    _ui->Render(recordState, Time::DeltaTimeSec());

    _displayRenderPass->End(recordState);

    LogicalDevice::EndCommandBuffer(recordState);
}

//======================================================================================================================

void BoidsSimulationApp::Resize()
{
    _sceneWindowResized = true;
}

//======================================================================================================================

void BoidsSimulationApp::Reload()
{
    LogicalDevice::DeviceWaitIdle();
}

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
    DisplayParametersWindow();
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

void BoidsSimulationApp::DisplayParametersWindow()
{
    _ui->BeginWindow("Parameters");

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

    ImGui::Checkbox("Enable separation force", &_config.simulation.bEnableSeparationForce);
    ImGui::Checkbox("Enable alignment force", &_config.simulation.bEnableAlignmentForce);
    ImGui::Checkbox("Enable cohesion force", &_config.simulation.bEnableCohesionForce);
    ImGui::Checkbox("Enable soft collision handling", &_config.simulation.bEnableSoftCollisionHandling);
    ImGui::Checkbox("Enable soft collision for boundary", &_config.simulation.bEnableSoftCollisionForBoundary);
    ImGui::Checkbox("Enable hard collision handling", &_config.simulation.bEnableHardCollisionHandling);
    ImGui::Checkbox("Enable boundary collision handling", &_config.simulation.bEnableBoundaryCollisionHandling);

	//--------------------------------------------------------------
	ImGui::Separator();
	//--------------------------------------------------------------

    ImGui::InputFloat("Separation radius", &_config.simulation.separationRadius);
    ImGui::InputFloat("Separation cos", &_config.simulation.separationCos);
    ImGui::InputFloat("Separation constant", &_config.simulation.separationConstant);

	//--------------------------------------------------------------
	ImGui::Separator();
	//--------------------------------------------------------------

    ImGui::InputFloat("Alignment radius", &_config.simulation.alignmentRadius);
    ImGui::InputFloat("Alignment cos", &_config.simulation.alignmentCos);
    ImGui::InputFloat("Alignment constant", &_config.simulation.alignmentConstant);


	//--------------------------------------------------------------
	ImGui::Separator();
	//--------------------------------------------------------------

    ImGui::InputFloat("Cohesion radius", &_config.simulation.cohesionRadius);
    ImGui::InputFloat("Cohesion cos", &_config.simulation.cohesionCos);
    ImGui::InputFloat("Cohesion constant", &_config.simulation.cohesionConstant);

	//--------------------------------------------------------------
	ImGui::Separator();
	//--------------------------------------------------------------

    ImGui::Checkbox("Clamp speed", &_config.simulation.bClampSpeed);
    ImGui::InputFloat("Min speed", &_config.simulation.minSpeed);
    ImGui::InputFloat("Max speed", &_config.simulation.maxSpeed);
    ImGui::Checkbox("Clamp acceleration", &_config.simulation.bClampAcc);
    ImGui::InputFloat("Min acceleration", &_config.simulation.minAcc);
    ImGui::InputFloat("Max acceleration", &_config.simulation.maxAcc);

	//--------------------------------------------------------------
	ImGui::Separator();
	//--------------------------------------------------------------

    ImGui::InputFloat("Soft collision offset", &_config.simulation.softCollisionOffset);
    ImGui::InputFloat("Hard collision offset", &_config.simulation.hardCollisionOffset);
    ImGui::InputFloat("Strict collision radius", &_config.simulation.strictCollisionOffset);

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
