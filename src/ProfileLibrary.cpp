#include "ProfileLibrary.h"
#include "IdGenerator.h"
#include "JsonStorage.h"

#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string_view>
#include <system_error>

namespace
{
    bool isSafeRelativePath(const std::filesystem::path& path)
    {
        if (path.is_absolute() || path.has_root_name() || path.has_root_directory())
        {
            return false;
        }

        for (const auto& component : path)
        {
            if (component == "..")
            {
                return false;
            }
        }

        return true;
    }

    bool isManagedAssetName(const std::filesystem::path& assetName)
    {
        return !assetName.empty() && !assetName.is_absolute() &&
            !assetName.has_parent_path() && assetName.filename() == assetName;
    }
}


ProfileLibrary::ProfileLibrary(
    const std::filesystem::path& libraryPath)
    : m_libraryPath(libraryPath),
    m_assetManager(libraryPath.parent_path())
{
    m_assetsPath =
        m_libraryPath.parent_path() / "Assets";

    std::filesystem::create_directories(
        m_libraryPath
    );

    std::filesystem::create_directories(
        m_assetsPath
    );
}


// Library information

const std::filesystem::path& ProfileLibrary::getLibraryPath() const
{
    return m_libraryPath;
}


// Asset Path
const std::filesystem::path& ProfileLibrary::getAssetsPath() const
{
    return m_assetsPath;
}


// Path handling

std::filesystem::path ProfileLibrary::resolvePath(
    const std::filesystem::path& path) const
{
    if (path.empty())
    {
        return m_libraryPath;
    }

    if (!isSafeRelativePath(path))
    {
        throw std::runtime_error(
            "Paths must be relative to the profile library."
        );
    }

    return (m_libraryPath / path).lexically_normal();
}


std::filesystem::path ProfileLibrary::getProfileFilePath(
    const std::filesystem::path& folder,
    const std::string& profileName) const
{
    validateProfileName(profileName);

    return resolvePath(folder) / (profileName + ".json");
}


std::filesystem::path ProfileLibrary::getAssetDirectory(
    const std::string& profileId
) const
{
    validateProfileId(profileId);

    return m_assetsPath / profileId;
}

void ProfileLibrary::validateProfileName(const std::string& profileName)
{
    const std::filesystem::path namePath(profileName);
    constexpr std::string_view invalidCharacters = "<>:\"/\\|?*";

    if (profileName.empty() || profileName == "." || profileName == ".." ||
        namePath.has_parent_path() || namePath.filename() != namePath ||
        std::any_of(profileName.begin(), profileName.end(), [&](unsigned char character)
        {
            return character < 32 || invalidCharacters.find(static_cast<char>(character)) != std::string_view::npos;
        }))
    {
        throw std::runtime_error("Profile name contains invalid filename characters.");
    }
}

void ProfileLibrary::validateProfileId(const std::string& profileId)
{
    if (profileId.size() != 8 || !std::all_of(profileId.begin(), profileId.end(), [](unsigned char character)
        {
            return std::isxdigit(character) != 0;
        }))
    {
        throw std::runtime_error("Profile contains an invalid ID.");
    }
}

// Folder operations

void ProfileLibrary::createFolder(
    const std::filesystem::path& path)
{
    if (path.empty())
    {
        throw std::runtime_error(
            "Folder path cannot be empty."
        );
    }

    const auto fullPath = resolvePath(path);

    if (std::filesystem::exists(fullPath))
    {
        throw std::runtime_error(
            "Folder already exists: " +
            fullPath.string()
        );
    }

    std::filesystem::create_directories(fullPath);
}


void ProfileLibrary::deleteFolder(
    const std::filesystem::path& path)
{
    if (path.empty())
    {
        throw std::runtime_error(
            "Cannot delete the library root."
        );
    }

    const auto fullPath = resolvePath(path);

    if (!std::filesystem::exists(fullPath))
    {
        throw std::runtime_error(
            "Folder does not exist: " +
            fullPath.string()
        );
    }

    if (!std::filesystem::is_directory(fullPath))
    {
        throw std::runtime_error(
            "Path is not a folder: " +
            fullPath.string()
        );
    }

    std::vector<std::filesystem::path> assetDirectories;

    // Remember managed assets before removing the profile files that reference them.
    for (const auto& entry : std::filesystem::recursive_directory_iterator(fullPath))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
        {
            continue;
        }

        try
        {
            assetDirectories.push_back(getAssetDirectory(JsonStorage::load(entry.path()).getId()));
        }
        catch (const std::exception&)
        {
            // A damaged JSON file should not prevent the folder itself from being deleted.
        }
    }

    std::filesystem::remove_all(fullPath);

    for (const auto& assetDirectory : assetDirectories)
    {
        std::error_code error;
        std::filesystem::remove_all(assetDirectory, error);
    }
}


bool ProfileLibrary::folderExists(
    const std::filesystem::path& path) const
{
    const auto fullPath = resolvePath(path);

    return std::filesystem::exists(fullPath) &&
        std::filesystem::is_directory(fullPath);
}


// Profile operations

void ProfileLibrary::createProfile(
    const std::filesystem::path& folder,
    const Profile& profile)
{
    if (profile.getName().empty())
    {
        throw std::runtime_error(
            "Profile name cannot be empty."
        );
    }

    const auto folderPath =
        resolvePath(folder);

    if (!std::filesystem::exists(folderPath))
    {
        throw std::runtime_error(
            "Profile folder does not exist: " +
            folderPath.string()
        );
    }

    if (!std::filesystem::is_directory(folderPath))
    {
        throw std::runtime_error(
            "Profile path is not a folder: " +
            folderPath.string()
        );
    }

    const auto profilePath =
        getProfileFilePath(
            folder,
            profile.getName()
        );

    if (std::filesystem::exists(profilePath))
    {
        throw std::runtime_error(
            "Profile already exists: " +
            profilePath.string()
        );
    }

    validateProfileName(profile.getName());

    Profile profileWithId = profile;

    profileWithId.setId(
        generateUniqueProfileId()
    );

    const auto assetDirectory =
        getAssetDirectory(
            profileWithId.getId()
        );

    std::filesystem::create_directories(
        assetDirectory
    );

    try
    {
        if (!profile.getAvatar().empty())
        {
            const auto destination =
                assetDirectory /
                ("avatar" +
                    profile.getAvatar().extension().string());

            m_assetManager.processAvatar(
                profile.getAvatar(),
                destination
            );

            profileWithId.setAvatar(
                destination.filename()
            );
        }

        if (!profile.getBanner().empty())
        {
            const auto destination =
                assetDirectory /
                ("banner" +
                    profile.getBanner().extension().string());

            m_assetManager.processBanner(
                profile.getBanner(),
                destination
            );

            profileWithId.setBanner(
                destination.filename()
            );
        }

        JsonStorage::save(
            profileWithId,
            profilePath
        );
    }
    catch (...)
    {
        std::filesystem::remove_all(
            assetDirectory
        );

        throw;
    }
}


Profile ProfileLibrary::loadProfile(
    const std::filesystem::path& path) const
{
    const auto fullPath = resolvePath(path);

    if (!std::filesystem::exists(fullPath))
    {
        throw std::runtime_error(
            "Profile does not exist: " +
            fullPath.string()
        );
    }

    if (!std::filesystem::is_regular_file(fullPath))
    {
        throw std::runtime_error(
            "Path is not a profile file: " +
            fullPath.string()
        );
    }

    if (fullPath.extension() != ".json")
    {
        throw std::runtime_error("Path is not a profile file: " + fullPath.string());
    }

    return JsonStorage::load(fullPath);
}


void ProfileLibrary::deleteProfile(
    const std::filesystem::path& path)
{
    const auto fullPath =
        resolvePath(path);

    if (!std::filesystem::exists(fullPath))
    {
        throw std::runtime_error(
            "Profile does not exist: " +
            fullPath.string()
        );
    }

    if (!std::filesystem::is_regular_file(fullPath))
    {
        throw std::runtime_error(
            "Path is not a profile file: " +
            fullPath.string()
        );
    }

    if (fullPath.extension() != ".json")
    {
        throw std::runtime_error("Path is not a profile file: " + fullPath.string());
    }

    // Load the profile so we can retrieve its ID.
    Profile profile =
        JsonStorage::load(fullPath);

    if (profile.getId().empty())
    {
        throw std::runtime_error(
            "Profile does not contain a valid ID: " +
            fullPath.string()
        );
    }

    const auto assetDirectory =
        getAssetDirectory(
            profile.getId()
        );

    std::filesystem::remove(fullPath);

    if (std::filesystem::exists(assetDirectory))
    {
        std::filesystem::remove_all(
            assetDirectory
        );
    }
}


void ProfileLibrary::moveProfile(
    const std::filesystem::path& from,
    const std::filesystem::path& to)
{
    const auto sourcePath =
        resolvePath(from);

    const auto destinationPath =
        resolvePath(to);

    // Check source
    if (!std::filesystem::exists(sourcePath))
    {
        throw std::runtime_error(
            "Source profile does not exist: " +
            sourcePath.string()
        );
    }

    if (!std::filesystem::is_regular_file(sourcePath))
    {
        throw std::runtime_error(
            "Source is not a profile file: " +
            sourcePath.string()
        );
    }

    if (sourcePath.extension() != ".json")
    {
        throw std::runtime_error(
            "Source is not a profile file: " +
            sourcePath.string()
        );
    }

    // Check destination
    if (std::filesystem::exists(destinationPath))
    {
        throw std::runtime_error(
            "Destination already exists: " +
            destinationPath.string()
        );
    }

    if (!std::filesystem::exists(
        destinationPath.parent_path()))
    {
        throw std::runtime_error(
            "Destination folder does not exist: " +
            destinationPath.parent_path().string()
        );
    }

    if (!std::filesystem::is_directory(
        destinationPath.parent_path()))
    {
        throw std::runtime_error(
            "Destination parent is not a folder: " +
            destinationPath.parent_path().string()
        );
    }

    // Move the profile JSON.
    std::filesystem::rename(
        sourcePath,
        destinationPath
    );
}


bool ProfileLibrary::profileExists(
    const std::filesystem::path& path) const
{
    const auto fullPath = resolvePath(path);

    return std::filesystem::exists(fullPath) &&
        std::filesystem::is_regular_file(fullPath) &&
        fullPath.extension() == ".json";
}


// --------------------------------------------------
// Tree listing
// --------------------------------------------------

void ProfileLibrary::printTree(
    const std::filesystem::path& path,
    int maxDepth) const
{
    const auto fullPath = resolvePath(path);

    if (!std::filesystem::exists(fullPath))
    {
        throw std::runtime_error(
            "Path does not exist: " +
            fullPath.string()
        );
    }

    if (!std::filesystem::is_directory(fullPath))
    {
        throw std::runtime_error(
            "Path is not a folder: " +
            fullPath.string()
        );
    }

    std::cout
        << fullPath.filename().string()
        << "/\n";

    printTreeRecursive(
        fullPath,
        0,
        maxDepth,
        ""
    );
}

std::string ProfileLibrary::generateUniqueProfileId() const
{
    while (true)
    {
        const std::string id =
            IdGenerator::generate();

        const auto assetDirectory =
            getAssetDirectory(id);

        if (!std::filesystem::exists(assetDirectory))
        {
            return id;
        }
    }
}


void ProfileLibrary::printTreeRecursive(
    const std::filesystem::path& path,
    int currentDepth,
    int maxDepth,
    const std::string& prefix) const
{
    if (maxDepth >= 0 &&
        currentDepth >= maxDepth)
    {
        return;
    }

    std::vector<std::filesystem::path> directories;
    std::vector<std::filesystem::path> profiles;

    for (const auto& entry :
        std::filesystem::directory_iterator(path))
    {
        if (std::filesystem::is_directory(entry))
        {
            directories.push_back(entry.path());
        }
        else if (
            std::filesystem::is_regular_file(entry) &&
            entry.path().extension() == ".json")
        {
            profiles.push_back(entry.path());
        }
    }

    std::sort(
        directories.begin(),
        directories.end()
    );

    std::sort(
        profiles.begin(),
        profiles.end()
    );

    const std::size_t totalEntries =
        directories.size() + profiles.size();

    std::size_t currentEntry = 0;

    for (const auto& directory : directories)
    {
        ++currentEntry;

        const bool isLast =
            currentEntry == totalEntries;

        std::cout
            << prefix
            << (isLast ? "└── " : "├── ")
            << directory.filename().string()
            << "/\n";

        const std::string childPrefix =
            prefix + (isLast ? "    " : "│   ");

        printTreeRecursive(
            directory,
            currentDepth + 1,
            maxDepth,
            childPrefix
        );
    }

    for (const auto& profile : profiles)
    {
        ++currentEntry;

        const bool isLast =
            currentEntry == totalEntries;

        std::cout
            << prefix
            << (isLast ? "└── " : "├── ")
            << profile.stem().string()
            << '\n';
    }
}

void ProfileLibrary::replaceAvatar(
    Profile& profile,
    const std::filesystem::path& source)
{
    const auto assetDirectory =
        getAssetDirectory(profile.getId());

    const auto destination =
        assetDirectory /
        ("avatar" +
            source.extension().string());

    const auto temporaryDestination =
        assetDirectory /
        ("avatar_new" +
            source.extension().string());

    try
    {
        // Keep the old avatar until the replacement was processed.
        m_assetManager.processAvatar(
            source,
            temporaryDestination
        );

        if (!profile.getAvatar().empty())
        {
            if (!isManagedAssetName(profile.getAvatar()))
            {
                throw std::runtime_error("Profile contains an invalid avatar filename.");
            }

            const auto oldAvatar =
                assetDirectory /
                profile.getAvatar();

            if (std::filesystem::exists(oldAvatar) &&
                oldAvatar != temporaryDestination)
            {
                std::filesystem::remove(
                    oldAvatar
                );
            }
        }

        std::filesystem::rename(
            temporaryDestination,
            destination
        );

        profile.setAvatar(
            destination.filename()
        );
    }
    catch (...)
    {
        if (std::filesystem::exists(
            temporaryDestination))
        {
            std::filesystem::remove(
                temporaryDestination
            );
        }

        throw;
    }
}

void ProfileLibrary::replaceBanner(
    Profile& profile,
    const std::filesystem::path& source)
{
    const auto assetDirectory =
        getAssetDirectory(profile.getId());

    const auto destination =
        assetDirectory /
        ("banner" +
            source.extension().string());

    const auto temporaryDestination =
        assetDirectory /
        ("banner_new" +
            source.extension().string());

    try
    {
        // Keep the old banner until the replacement was processed.
        m_assetManager.processBanner(
            source,
            temporaryDestination
        );

        if (!profile.getBanner().empty())
        {
            if (!isManagedAssetName(profile.getBanner()))
            {
                throw std::runtime_error("Profile contains an invalid banner filename.");
            }

            const auto oldBanner =
                assetDirectory /
                profile.getBanner();

            if (std::filesystem::exists(oldBanner) &&
                oldBanner != temporaryDestination)
            {
                std::filesystem::remove(
                    oldBanner
                );
            }
        }

        std::filesystem::rename(
            temporaryDestination,
            destination
        );

        profile.setBanner(
            destination.filename()
        );
    }
    catch (...)
    {
        if (std::filesystem::exists(
            temporaryDestination))
        {
            std::filesystem::remove(
                temporaryDestination
            );
        }

        throw;
    }
}

void ProfileLibrary::editProfile(
    const std::filesystem::path& path,
    const Profile& changes)
{
    const auto fullPath =
        resolvePath(path);

    if (!std::filesystem::exists(fullPath))
    {
        throw std::runtime_error(
            "Profile does not exist: " +
            fullPath.string()
        );
    }

    if (!std::filesystem::is_regular_file(fullPath))
    {
        throw std::runtime_error(
            "Path is not a profile file: " +
            fullPath.string()
        );
    }

    Profile profile =
        JsonStorage::load(fullPath);

    if (profile.getId().empty())
    {
        throw std::runtime_error(
            "Profile does not contain a valid ID."
        );
    }

    if (!changes.getName().empty())
    {
        validateProfileName(changes.getName());
        profile.setName(changes.getName());
    }

    if (!changes.getPrimaryColor().empty())
        profile.setPrimaryColor(
            changes.getPrimaryColor()
        );

    if (!changes.getSecondaryColor().empty())
        profile.setSecondaryColor(
            changes.getSecondaryColor()
        );

    if (!changes.getNameplate().empty())
        profile.setNameplate(
            changes.getNameplate()
        );

    if (!changes.getAvatarDecoration().empty())
        profile.setAvatarDecoration(
            changes.getAvatarDecoration()
        );

    if (!changes.getDisplayNameStyle().empty())
        profile.setDisplayNameStyle(
            changes.getDisplayNameStyle()
        );

    if (!changes.getProfileEffect().empty())
        profile.setProfileEffect(
            changes.getProfileEffect()
        );

    if (!changes.getProfileFrame().empty())
        profile.setProfileFrame(
            changes.getProfileFrame()
        );

    if (!changes.getAvatar().empty())
    {
        replaceAvatar(
            profile,
            changes.getAvatar()
        );
    }

    if (!changes.getBanner().empty())
    {
        replaceBanner(
            profile,
            changes.getBanner()
        );
    }

    JsonStorage::save(
        profile,
        fullPath
    );
}
