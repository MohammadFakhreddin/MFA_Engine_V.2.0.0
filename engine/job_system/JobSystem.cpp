#include "JobSystem.hpp"

//----------------------------------------------------------------------------------------------------------------------

std::shared_ptr<mfa::JobSystem> mfa::JobSystem::Instantiate()
{
    std::shared_ptr<mfa::JobSystem> shared_ptr = _instance.lock();
    if (shared_ptr == nullptr)
    {
        shared_ptr = std::make_shared<mfa::JobSystem>();
        _instance = shared_ptr;
    }
    return shared_ptr;
}

//----------------------------------------------------------------------------------------------------------------------

void mfa::JobSystem::Destroy()
{
    _instance.reset();
}

//----------------------------------------------------------------------------------------------------------------------

bool mfa::JobSystem::HasInstance() { return !_instance.expired(); }

//----------------------------------------------------------------------------------------------------------------------

mfa::JobSystem::JobSystem() = default;
mfa::JobSystem::~JobSystem() = default;

//----------------------------------------------------------------------------------------------------------------------