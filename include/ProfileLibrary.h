#pragma once

#include <filesystem>
#include <vector>
#include <string>

#include "Profile.h"
#include "AssetManager.h"

class ProfileLibrary
{
public:
    explicit ProfileLibrary(
        const std::filesystem::path& libraryPath = "ProfileLibrary"
    );

    // Asset Path
    const std::filesystem::path& getAssetsPath() const;

    // Library information
    const std::filesystem::path& getLibraryPath() const;

    // Folder operations
    void createFolder(const std::filesystem::path& path);
    void deleteFolder(const std::filesystem::path& path);

    bool folderExists(const std::filesystem::path& path) const;

    // Profile operations
    void createProfile(
        const std::filesystem::path& folder,
        const Profile& profile
    );

    Profile loadProfile(
        const std::filesystem::path& path
    ) const;

    void deleteProfile(
        const std::filesystem::path& path
    );

    void moveProfile(
        const std::filesystem::path& from,
        const std::filesystem::path& to
    );

    bool profileExists(
        const std::filesystem::path& path
    ) const;

    // Listing
    std::vector<std::filesystem::path> list(
        const std::filesystem::path& path = ""
    ) const;

    void printTree(
        const std::filesystem::path& path = "",
        int maxDepth = -1
    ) const;

    void editProfile(
        const std::filesystem::path& path,
        const Profile& changes
    );

private:
    std::filesystem::path m_libraryPath;


    std::filesystem::path resolvePath(
        const std::filesystem::path& path
    ) const;

    std::filesystem::path getProfileFilePath(
        const std::filesystem::path& folder,
        const std::string& profileName
    ) const;

    void printTreeRecursive(
        const std::filesystem::path& path,
        int currentDepth,
        int maxDepth,
        const std::string& prefix
    ) const;

    std::string generateUniqueProfileId() const;

    std::filesystem::path m_assetsPath;

    std::filesystem::path getAssetDirectory(
        const std::string& profileId
    ) const;

    static void validateProfileName(const std::string& profileName);
    static void validateProfileId(const std::string& profileId);

    void replaceAvatar(
        Profile& profile,
        const std::filesystem::path& source
    );

    void replaceBanner(
        Profile& profile,
        const std::filesystem::path& source
    );

    AssetManager m_assetManager;
};
