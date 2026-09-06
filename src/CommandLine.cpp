#include "CommandLine.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <iostream>


CommandLine::CommandLine(
    int argc,
    char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string argument = argv[i];

        // Every command-line parameter must start with '-'.
        if (argument.empty() || argument[0] != '-')
        {
            throw std::runtime_error(
                "Unexpected argument: " + argument
            );
        }

        // Remove the leading '-'.
        std::string name = argument.substr(1);

        if (name.empty())
        {
            throw std::runtime_error(
                "Empty command-line parameter."
            );
        }

        name = normalize(name);

        // Don't allow the same parameter twice.
        if (m_arguments.contains(name))
        {
            throw std::runtime_error(
                "Parameter specified more than once: -" + name
            );
        }

        // Check whether the next argument is a value.
        if (i + 1 < argc)
        {
            std::string next = argv[i + 1];

            if (!next.empty() && next[0] != '-')
            {
                m_arguments[name] = next;
                ++i;

                continue;
            }
        }

        // No value means this is a flag.
        m_arguments[name] = "";
    }
}


// Parameter lookup

bool CommandLine::has(
    const std::string& name) const
{
    const std::string normalizedName =
        normalize(name);

    return m_arguments.contains(normalizedName);
}


bool CommandLine::hasValue(
    const std::string& name) const
{
    const std::string normalizedName =
        normalize(name);

    auto it = m_arguments.find(normalizedName);

    if (it == m_arguments.end())
    {
        return false;
    }

    return !it->second.empty();
}


// Get values

std::string CommandLine::getRequired(
    const std::string& name) const
{
    const std::string normalizedName =
        normalize(name);

    auto it = m_arguments.find(normalizedName);

    if (it == m_arguments.end())
    {
        throw std::runtime_error(
            "Missing required parameter: -" + name
        );
    }

    if (it->second.empty())
    {
        throw std::runtime_error(
            "Parameter requires a value: -" + name
        );
    }

    return it->second;
}


std::string CommandLine::getOptional(
    const std::string& name,
    const std::string& defaultValue) const
{
    const std::string normalizedName =
        normalize(name);

    auto it = m_arguments.find(normalizedName);

    if (it == m_arguments.end())
    {
        return defaultValue;
    }

    return it->second;
}


// Normalization

std::string CommandLine::normalize(
    const std::string& name)
{
    std::string result = name;

    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(
                std::tolower(c)
                );
        }
    );

    return result;
}

void CommandLine::printHelp()
{
    std::cout
        << "\n"
        << "Discord Profile Tool\n"
        << "====================\n\n"

        << "Usage:\n"
        << "  DcProfilesTool.exe -Command [parameters]\n\n"

        << "Folder Commands:\n"
        << "  -CreateFolder\n"
        << "      -Path <path>\n"
        << "      Create a new folder.\n\n"

        << "  -DeleteFolder\n"
        << "      -Path <path>\n"
        << "      Delete a folder and everything inside it.\n\n"

        << "  -List\n"
        << "      [-Path <path>]\n"
        << "      [-Depth <number> | -All]\n"
        << "      Display the profile library as a tree.\n"
        << "      Default depth: 4\n\n"

        << "Profile Commands:\n"
        << "  -CreateProfile\n"
        << "      -Path <folder>\n"
        << "      -Name <name>\n"
        << "      [-Avatar <file>]\n"
        << "      [-Banner <file>]\n"
        << "      [-PrimaryColor <color>]\n"
        << "      [-SecondaryColor <color>]\n"
        << "      [-Nameplate <value>]\n"
        << "      [-AvatarDecoration <value>]\n"
        << "      [-DisplayNameStyle <value>]\n"
        << "      [-ProfileEffect <value>]\n"
        << "      [-ProfileFrame <value>]\n"
        << "      Create a new profile.\n\n"

        << "  -ShowProfile\n"
        << "      -Path <profile>\n"
        << "      Display profile information.\n\n"

        << "  -EditProfile\n"
        << "      -Path <profile>\n"
        << "      [profile parameters]\n"
        << "      Edit an existing profile.\n\n"

        << "  -DeleteProfile\n"
        << "      -Path <profile>\n"
        << "      Delete a profile and its assets.\n\n"

        << "  -MoveProfile\n"
        << "      -Path <profile>\n"
        << "      -Destination <profile>\n"
        << "      Move a profile to another folder.\n\n"

        << "General:\n"
        << "  -Help\n"
        << "      Display this help message.\n\n";
}