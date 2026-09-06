#pragma once

#include <filesystem>

#include "Profile.h"

class JsonStorage
{
public:
    static void save(const Profile& profile,
        const std::filesystem::path& filePath);

    static Profile load(const std::filesystem::path& filePath);
};