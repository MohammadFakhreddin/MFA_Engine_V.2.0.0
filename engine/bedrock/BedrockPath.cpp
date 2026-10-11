#include "BedrockPath.hpp"

#include "BedrockAssert.hpp"
#include "BedrockCommon.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

static bool LogCalledOnce = false;

//-------------------------------------------------------------------------------------------------

std::unique_ptr<mfa::Path> mfa::Path::Init()
{
    MFA_ASSERT(_instance == nullptr);
	return std::make_unique<mfa::Path>();
}

//-------------------------------------------------------------------------------------------------

mfa::Path::Path()
{
    _instance = this;
#if defined(ASSET_DIR)
	mAssetPath = std::filesystem::absolute(std::string(TO_LITERAL(ASSET_DIR))).string();
#endif

	static constexpr char const * OVERRIDE_ASSET_PATH = "./asset_dir.txt";
	if (std::filesystem::exists(OVERRIDE_ASSET_PATH))
	{
		std::ifstream nameFileout{};
		mAssetPath.clear();

		nameFileout.open(OVERRIDE_ASSET_PATH);
		while (nameFileout >> mAssetPath)
		{
			std::cout << mAssetPath;
		}
		nameFileout.close();
	    if (LogCalledOnce == false)
	    {
	        MFA_LOG_INFO("Override asset path is %s", mAssetPath.c_str());
	    }
	}
	else
	{
	    if (LogCalledOnce == false)
	    {
	        MFA_LOG_INFO("No override found, using the default directory: %s", mAssetPath.c_str());
	    }
	}

    LogCalledOnce = true;
}

//-------------------------------------------------------------------------------------------------

mfa::Path::~Path()
{
    MFA_ASSERT(_instance == this);
    _instance = nullptr;
};

//-------------------------------------------------------------------------------------------------

std::string mfa::Path::Get(std::string const & address)
{
	return Get(address.c_str());
}

//-------------------------------------------------------------------------------------------------

std::string mfa::Path::Get(char const * address)
{
    if (_instance == nullptr)
    {
        return "";
    }

    if (std::strncmp(address, "./", 2) == 0 ||
        std::strncmp(address, "/", 1) == 0 ||
        std::strncmp(address, "../", 3) == 0)

    {
        return address;
    }

    return std::filesystem::path(_instance->mAssetPath).append(address).string();
}

//-------------------------------------------------------------------------------------------------

std::string mfa::Path::Get(char const * address, char const * relativePath)
{
    if (std::strncmp(address, "./", 2) == 0 ||
        std::strncmp(address, "/", 1) == 0 ||
        std::strncmp(address, "../", 3) == 0)
    {
        return address;
    }
    return std::filesystem::path(relativePath).append(address).string();
}

//-------------------------------------------------------------------------------------------------

std::string mfa::Path::Relative(char const *address)
{
    if (_instance == nullptr)
    {
        return "";
    }
    if (std::strncmp(address, _instance->mAssetPath.c_str(), _instance->mAssetPath.size()) == 0)
    {
        return std::string(address).substr(_instance->mAssetPath.size());
    }
    return address;
}

//-------------------------------------------------------------------------------------------------

std::string mfa::Path::AssetPath()
{
    if (_instance == nullptr)
    {
        return "";
    }
    return _instance->mAssetPath;
}

//-------------------------------------------------------------------------------------------------
