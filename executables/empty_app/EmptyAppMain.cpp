//
// Created by mohammad on 2026-03-08.
//

#include "BedrockLog.hpp"
#include "BedrockPath.hpp"
#include "EmptyApp.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

int main()
{
    LogicalDevice::InitParams params{.windowWidth = 1920,
                                     .windowHeight = 1080,
                                     .resizable = true,
                                     .fullScreen = false,
                                     .applicationName = "Empty sample"};

    auto device = LogicalDevice::Init(params);
    assert(device->IsValid() == true);
    {
        auto path = Path::Init();

        EmptyApp app{};
        app.Run();
    }

    return 0;
}