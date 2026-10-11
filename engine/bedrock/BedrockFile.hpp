#pragma once

#include <string>

#include "BedrockMemory.hpp"

namespace mfa::File
{
    std::shared_ptr<Blob> Read(std::string const & path);
}