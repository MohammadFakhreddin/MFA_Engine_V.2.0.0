#pragma once

#include "BedrockAssert.hpp"
#include "BedrockCommon.hpp"
#include "BedrockPlatforms.hpp"

#include <atomic>

namespace mfa {
    class ScopeProfiler
    {
    public:
        explicit ScopeProfiler(std::string text);
        ~ScopeProfiler();

        ScopeProfiler(ScopeProfiler const &) noexcept = delete;
        ScopeProfiler(ScopeProfiler &&) noexcept = delete;
        ScopeProfiler & operator = (ScopeProfiler const &) noexcept = delete;
        ScopeProfiler & operator = (ScopeProfiler &&) noexcept = delete;

    private:

        std::string const _text;
        supportedClock::time_point _start{};
    };
}

#define MFA_SCOPE_Profiler(lock)        mfa::ScopeProfiler MFA_UNIQUE_NAME(__scopeProfiler) {lock};
