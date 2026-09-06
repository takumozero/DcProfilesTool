#include <iostream>
#include <filesystem>
#include <algorithm>
#include <array>
#include <string_view>
#include <stdexcept>

#include "CommandLine.h"
#include "Profile.h"
#include "ProfileLibrary.h"
#include "AssetManager.h"

#ifdef _WIN32
#include <windows.h>
#endif



int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    try
    {
        CommandLine commandLine(argc, argv);

        const std::array<std::string_view, 9> commands = {
            "list", "createfolder", "deletefolder", "showprofile", "moveprofile",
            "deleteprofile", "createprofile", "editprofile", "help"
        };

        const auto commandCount = std::count_if(commands.begin(), commands.end(),
            [&commandLine](std::string_view command)
            {
                return commandLine.has(std::string(command));
            });

        if (commandCount > 1)
        {
            throw std::runtime_error("Specify only one command at a time.");
        }

        const auto executablePath =
            std::filesystem::absolute(argv[0]);

        const auto executableDirectory =
            executablePath.parent_path();

        const auto libraryPath =
            executableDirectory / "ProfileLibrary";

        ProfileLibrary library(libraryPath);

        if (commandLine.has("List"))
        {
            const std::string path =
                commandLine.getOptional("Path");

            int depth = 4;


            if (commandLine.has("All"))
            {
                if (commandLine.has("Depth"))
                {
                    throw std::runtime_error(
                        "-Depth and -All cannot be used together."
                    );
                }

                depth = -1;
            }


            else if (commandLine.hasValue("Depth"))
            {
                depth = std::stoi(
                    commandLine.getRequired("Depth")
                );

                if (depth < 0)
                {
                    throw std::runtime_error(
                        "Depth cannot be negative."
                    );
                }
            }


            library.printTree(
                path,
                depth
            );

            return 0;
        }


        // --------------------------------------------------
        // Create Folder
        // --------------------------------------------------

        if (commandLine.has("CreateFolder"))
        {
            const std::string path =
                commandLine.getRequired("Path");

            library.createFolder(path);

            std::cout
                << "Folder created successfully.\n";

            std::cout
                << "Path: "
                << path
                << '\n';

            return 0;
        }


        // --------------------------------------------------
        // Delete Folder
        // --------------------------------------------------

        if (commandLine.has("DeleteFolder"))
        {
            const std::string path =
                commandLine.getRequired("Path");

            std::cout
                << "Are you sure you want to delete the folder:\n"
                << "  " << path << "\n"
                << "This will delete the folder and everything inside it.\n"
                << "Type 'y' to confirm, or anything else to cancel: ";

            std::string confirmation;
            std::getline(std::cin, confirmation);

            if (confirmation != "y")
            {
                std::cout
                    << "Deletion cancelled.\n";

                return 0;
            }

            library.deleteFolder(path);

            std::cout
                << "Folder deleted successfully.\n";

            std::cout
                << "Path: "
                << path
                << '\n';

            return 0;
        }


        // --------------------------------------------------
        // Show Profile
        // --------------------------------------------------

        if (commandLine.has("ShowProfile"))
        {
            const std::string path =
                commandLine.getRequired("Path");

            Profile profile =
                library.loadProfile(path);

            std::cout
                << "\n"
                << "ID: "
                << profile.getId()
                << "\n";

            std::cout
                << "\n"
                << "Profile: "
                << profile.getName()
                << "\n\n";

            std::cout
                << "Avatar:             "
                << profile.getAvatar().string()
                << '\n';

            std::cout
                << "Banner:             "
                << profile.getBanner().string()
                << '\n';

            std::cout
                << '\n';

            std::cout
                << "Primary Color:      "
                << profile.getPrimaryColor()
                << '\n';

            std::cout
                << "Secondary Color:    "
                << profile.getSecondaryColor()
                << '\n';

            std::cout
                << '\n';

            std::cout
                << "Nameplate:          "
                << profile.getNameplate()
                << '\n';

            std::cout
                << "Avatar Decoration:  "
                << profile.getAvatarDecoration()
                << '\n';

            std::cout
                << "Display Name Style: "
                << profile.getDisplayNameStyle()
                << '\n';

            std::cout
                << "Profile Effect:     "
                << profile.getProfileEffect()
                << '\n';

            std::cout
                << "Profile Frame:      "
                << profile.getProfileFrame()
                << '\n';

            return 0;
        }

        // --------------------------------------------------
        // Move Profile
        // --------------------------------------------------

        if (commandLine.has("MoveProfile"))
        {
            const std::string path =
                commandLine.getRequired("Path");

            const std::string destination =
                commandLine.getRequired("Destination");

            // Load the profile first so we can show its name.
            Profile profile =
                library.loadProfile(path);

            std::cout
                << "Are you sure you want to move the profile:\n"
                << "  " << profile.getName() << "\n"
                << "From: " << path << "\n"
                << "To:   " << destination << "\n"
                << "Type 'y' to confirm, or anything else to cancel: ";

            std::string confirmation;
            std::getline(std::cin, confirmation);

            if (confirmation != "y")
            {
                std::cout
                    << "Move cancelled.\n";

                return 0;
            }

            library.moveProfile(
                path,
                destination
            );

            std::cout
                << "Profile moved successfully.\n";

            std::cout
                << "Profile: "
                << profile.getName()
                << '\n';

            std::cout
                << "New path: "
                << destination
                << '\n';

            return 0;
        }


        // --------------------------------------------------
        // Delete Profile
        // --------------------------------------------------

        if (commandLine.has("DeleteProfile"))
        {
            const std::string path =
                commandLine.getRequired("Path");

            // Load the profile first so we can show its name.
            Profile profile =
                library.loadProfile(path);

            std::cout
                << "Are you sure you want to delete the profile:\n"
                << "  " << profile.getName() << "\n"
                << "Path: " << path << "\n"
                << "Type 'y' to confirm, or anything else to cancel: ";

            std::string confirmation;
            std::getline(std::cin, confirmation);

            if (confirmation != "y")
            {
                std::cout
                    << "Deletion cancelled.\n";

                return 0;
            }

            library.deleteProfile(path);

            std::cout
                << "Profile deleted successfully.\n";

            std::cout
                << "Profile: "
                << profile.getName()
                << '\n';

            return 0;
        }


        // --------------------------------------------------
        // Create Profile
        // --------------------------------------------------

        if (commandLine.has("CreateProfile"))
        {
            const std::string path =
                commandLine.getRequired("Path");

            const std::string name =
                commandLine.getRequired("Name");


            Profile profile(name);


            // Optional fields

            if (commandLine.hasValue("Avatar"))
            {
                profile.setAvatar(
                    commandLine.getOptional("Avatar")
                );
            }

            if (commandLine.hasValue("Banner"))
            {
                profile.setBanner(
                    commandLine.getOptional("Banner")
                );
            }

            if (commandLine.hasValue("PrimaryColor"))
            {
                profile.setPrimaryColor(
                    commandLine.getOptional("PrimaryColor")
                );
            }

            if (commandLine.hasValue("SecondaryColor"))
            {
                profile.setSecondaryColor(
                    commandLine.getOptional("SecondaryColor")
                );
            }

            if (commandLine.hasValue("Nameplate"))
            {
                profile.setNameplate(
                    commandLine.getOptional("Nameplate")
                );
            }

            if (commandLine.hasValue("AvatarDecoration"))
            {
                profile.setAvatarDecoration(
                    commandLine.getOptional("AvatarDecoration")
                );
            }

            if (commandLine.hasValue("DisplayNameStyle"))
            {
                profile.setDisplayNameStyle(
                    commandLine.getOptional("DisplayNameStyle")
                );
            }

            if (commandLine.hasValue("ProfileEffect"))
            {
                profile.setProfileEffect(
                    commandLine.getOptional("ProfileEffect")
                );
            }

            if (commandLine.hasValue("ProfileFrame"))
            {
                profile.setProfileFrame(
                    commandLine.getOptional("ProfileFrame")
                );
            }


            // Create the profile file
            library.createProfile(
                path,
                profile
            );


            std::cout
                << "Profile created successfully.\n";

            std::cout
                << "Path: "
                << path
                << "/"
                << name
                << ".json\n";

            return 0;
        }

        // --------------------------------------------------
        // Edit Profile
        // --------------------------------------------------

        if (commandLine.has("EditProfile"))
        {
            const std::string path =
                commandLine.getRequired("Path");

            Profile changes;

            if (commandLine.hasValue("Name"))
            {
                changes.setName(
                    commandLine.getRequired("Name")
                );
            }

            if (commandLine.hasValue("Avatar"))
            {
                changes.setAvatar(
                    commandLine.getRequired("Avatar")
                );
            }

            if (commandLine.hasValue("Banner"))
            {
                changes.setBanner(
                    commandLine.getRequired("Banner")
                );
            }

            if (commandLine.hasValue("PrimaryColor"))
            {
                changes.setPrimaryColor(
                    commandLine.getRequired("PrimaryColor")
                );
            }

            if (commandLine.hasValue("SecondaryColor"))
            {
                changes.setSecondaryColor(
                    commandLine.getRequired("SecondaryColor")
                );
            }

            if (commandLine.hasValue("Nameplate"))
            {
                changes.setNameplate(
                    commandLine.getRequired("Nameplate")
                );
            }

            if (commandLine.hasValue("AvatarDecoration"))
            {
                changes.setAvatarDecoration(
                    commandLine.getRequired("AvatarDecoration")
                );
            }

            if (commandLine.hasValue("DisplayNameStyle"))
            {
                changes.setDisplayNameStyle(
                    commandLine.getRequired("DisplayNameStyle")
                );
            }

            if (commandLine.hasValue("ProfileEffect"))
            {
                changes.setProfileEffect(
                    commandLine.getRequired("ProfileEffect")
                );
            }

            if (commandLine.hasValue("ProfileFrame"))
            {
                changes.setProfileFrame(
                    commandLine.getRequired("ProfileFrame")
                );
            }

            std::cout
                << "Are you sure you want to edit the profile:\n"
                << "  " << path << "\n"
                << "Type 'y' to confirm, or anything else to cancel: ";

            std::string confirmation;
            std::getline(std::cin, confirmation);

            if (confirmation != "y")
            {
                std::cout
                    << "Edit cancelled.\n";

                return 0;
            }

            library.editProfile(
                path,
                changes
            );

            std::cout
                << "Profile edited successfully.\n";

            return 0;
        }

        // --------------------------------------------------
        // Help
        // --------------------------------------------------

        if (commandLine.has("Help"))
        {
            CommandLine::printHelp();
            return 0;
        }


        // --------------------------------------------------
        // No command
        // --------------------------------------------------

        std::cout
            << "No command specified. Use -Help for help.\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Error: "
            << e.what()
            << '\n';

        return 1;
    }
}
