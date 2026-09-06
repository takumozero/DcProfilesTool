#include "JsonStorage.h"

#include <fstream>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#endif

#include <nlohmann/json.hpp>

using json = nlohmann::json;


void JsonStorage::save(
    const Profile& profile,
    const std::filesystem::path& filePath)
{
    json j;

    j["id"] = profile.getId();

    j["name"] = profile.getName();

    j["avatar"] = profile.getAvatar().string();
    j["banner"] = profile.getBanner().string();

    j["primary_color"] = profile.getPrimaryColor();
    j["secondary_color"] = profile.getSecondaryColor();

    j["nameplate"] = profile.getNameplate();
    j["avatar_decoration"] = profile.getAvatarDecoration();
    j["display_name_style"] = profile.getDisplayNameStyle();
    j["profile_effect"] = profile.getProfileEffect();
    j["profile_frame"] = profile.getProfileFrame();


    const auto temporaryPath = filePath.string() + ".tmp";
    std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Could not open file for writing: " +
            filePath.string()
        );
    }

    file << j.dump(4) << '\n';

    if (!file)
    {
        file.close();
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Could not write profile file: " + filePath.string());
    }

    file.close();

    if (!file)
    {
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Could not finish writing profile file: " + filePath.string());
    }

#ifdef _WIN32
    if (!MoveFileExW(
        std::filesystem::path(temporaryPath).c_str(), filePath.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Could not replace profile file: " + filePath.string());
    }
#else
    std::error_code error;
    std::filesystem::rename(temporaryPath, filePath, error);
    if (error)
    {
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Could not replace profile file: " + filePath.string());
    }
#endif
}


Profile JsonStorage::load(
    const std::filesystem::path& filePath)
{
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Could not open profile file: " +
            filePath.string()
        );
    }

    json j;
    try
    {
        file >> j;
    }
    catch (const json::exception& error)
    {
        throw std::runtime_error("Invalid profile JSON in " + filePath.string() + ": " + error.what());
    }

    file.close();


    Profile profile;

    try
    {
        profile.setId(j.at("id").get<std::string>());

    profile.setName(j.at("name").get<std::string>());

    profile.setAvatar(
        j.at("avatar").get<std::string>()
    );

    profile.setBanner(
        j.at("banner").get<std::string>()
    );

    profile.setPrimaryColor(
        j.at("primary_color").get<std::string>()
    );

    profile.setSecondaryColor(
        j.at("secondary_color").get<std::string>()
    );

    profile.setNameplate(
        j.at("nameplate").get<std::string>()
    );

    profile.setAvatarDecoration(
        j.at("avatar_decoration").get<std::string>()
    );

    profile.setDisplayNameStyle(
        j.at("display_name_style").get<std::string>()
    );

    profile.setProfileEffect(
        j.at("profile_effect").get<std::string>()
    );

        profile.setProfileFrame(j.at("profile_frame").get<std::string>());
    }
    catch (const json::exception& error)
    {
        throw std::runtime_error("Invalid profile fields in " + filePath.string() + ": " + error.what());
    }

    return profile;
}
