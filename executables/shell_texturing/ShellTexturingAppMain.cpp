//
// Created by mohammad on 2026-03-08.
//

#include "BedrockLog.hpp"
#include "BedrockPath.hpp"
#include "ShellTexturingApp.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

int main()
{
    LogicalDevice::InitParams params{.windowWidth = 1920,
                                     .windowHeight = 1080,
                                     .resizable = true,
                                     .fullScreen = false,
                                     .applicationName = "Shell Texturing"};

    auto device = LogicalDevice::Init(params);
    assert(device->IsValid() == true);
    {
        auto path = Path::Init();

        ShellTexturingApp app{};
        app.Run();
    }

    return 0;
}
