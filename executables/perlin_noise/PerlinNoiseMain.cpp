//
// Created by mohammad on 2026-03-08.
//

#include "BedrockLog.hpp"
#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"
#include "PerlinNoiseApp.hpp"

using namespace MFA;

int main()
{
    LogicalDevice::InitParams params{.windowWidth = 1920,
                                     .windowHeight = 1080,
                                     .resizable = true,
                                     .fullScreen = false,
                                     .applicationName = "PerlinNoise"};

    auto device = LogicalDevice::Init(params);
    assert(device->IsValid() == true);
    {
        auto path = Path::Init();

        PerlinNoiseApp app{};
        app.Run();
    }

    return 0;
}