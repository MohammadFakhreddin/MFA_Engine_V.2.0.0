#pragma once

#include "BedrockPlatforms.hpp"
#include "BedrockString.hpp"

#include <cstdarg>
#include <string>


namespace mfa::Log {

    void Debug(char const * message, ...);

    void Info(char const * message, ...);

    void Warn(char const * message, ...);

    void Error(char const * message, ...);

    void _Debug(char const * file, int line, char const * function, char const * message, ...);

    void _Info(char const * file, int line, char const * function, char const * message, ...);

    void _Warn(char const * file, int line, char const * function, char const * message, ...);

    void _Error(char const * file, int line, char const * function, char const * message, ...);

} // mfa::Log


#ifdef MFA_DEBUG
    #define MFA_LOG_DEBUG(fmt_, ...)                mfa::Log::_Debug(__FILE__, __LINE__, __FUNCTION__, fmt_, ##__VA_ARGS__)
#else
    #define MFA_LOG_DEBUG(fmt_, ...)                         
#endif

#define MFA_LOG_INFO(fmt_, ...)                 mfa::Log::_Info(__FILE__, __LINE__, __FUNCTION__, fmt_, ##__VA_ARGS__)
#define MFA_LOG_WARN(fmt_, ...)                 mfa::Log::_Warn(__FILE__, __LINE__, __FUNCTION__, fmt_, ##__VA_ARGS__)
#define MFA_LOG_ERROR(fmt_, ...)                mfa::Log::_Error(__FILE__, __LINE__, __FUNCTION__, fmt_, ##__VA_ARGS__)
