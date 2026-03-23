//
// Created by mohammad on 2026-03-08.
//

#include "BedrockPath.hpp"
#include "BoidsApp.hpp"
#include "JobSystem.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

int main()
{
    char const * extensions[] = {VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME};

    LogicalDevice::InitParams params{.windowWidth = 1920,
                                     .windowHeight = 1080,
                                     .resizable = true,
                                     .fullScreen = false,
                                     .applicationName = "Boids",
                                     .extensionCount = 0,
                                     .extensions = extensions};

    auto device = LogicalDevice::Init(params);
    assert(device->IsValid() == true);
    {
        auto path = Path::Init();
        auto jobSystem = JobSystem::Instantiate();

        BoidsSimulationApp app{};
        app.Run();
    }

    return 0;
}
