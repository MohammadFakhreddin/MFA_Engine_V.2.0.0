#pragma once

#include "BedrockPlatforms.hpp"
#include "BedrockLog.hpp"

#include <stdio.h>
#include <stdexcept>
#include <cassert>

#ifdef __PLATFORM_WIN__

#define MFA_ASSUME(condition)           __assume(condition)

#ifdef MFA_DEBUG
#define MFA_ASSERT(condition)           assert(condition); MFA_ASSUME(condition)
#else
#define MFA_ASSERT(condition)           MFA_ASSUME(condition)
#endif

#define MFA_REQUIRE(condition)          if(!(condition)) {MFA_CRASH("Condition failed");}; MFA_ASSUME(condition)

#define MFA_CRASH(message, ...)         MFA_LOG_ERROR(message, __VA_ARGS__); throw std::runtime_error(message)


#else

#define MFA_ASSUME(condition)

#ifdef MFA_DEBUG
#define MFA_ASSERT(condition)           assert(condition);
#else
#define MFA_ASSERT(condition)           
#endif

#define MFA_REQUIRE(condition)          if(!(condition)) {MFA_CRASH("Condition failed");};

#define MFA_CRASH(message, ...)         throw std::runtime_error(message)// TODO MFA_LOG_ERROR(message, __VA_ARGS__); throw std::runtime_error(message)


#endif

#define MFA_NOT_IMPLEMENTED_YET(who)    MFA_LOG_ERROR(who); throw std::runtime_error("Method not implemented by " + std::string(who))

bool constexpr MFA_VERIFY(bool const condition) {
    if (condition) {
        return true;
    }
#ifdef MFA_DEBUG
    throw std::runtime_error("Condition failed");
#endif
    return false;
}

namespace mfa::Assert {} // namespace mfa::Assert
