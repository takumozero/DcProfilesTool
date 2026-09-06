#pragma once

#include <string>
#include <unordered_map>

class CommandLine
{
public:
    CommandLine(int argc, char* argv[]);

    // Check whether a parameter/flag exists.
    bool has(const std::string& name) const;

    // Check whether a parameter has a value.
    bool hasValue(const std::string& name) const;

    // Get a parameter value.
    // Throws an exception if the parameter doesn't exist
    // or has no value.
    std::string getRequired(const std::string& name) const;

    // Get a parameter value, or return the default value.
    std::string getOptional(
        const std::string& name,
        const std::string& defaultValue = ""
    ) const;

    static void printHelp();

private:
    // Parameter -> value
    //
    // Example:
    // -Path "Games/Cyberpunk 2077"
    //
    // becomes:
    // "path" -> "Games/Cyberpunk 2077"
    //
    // Flags such as -CreateProfile have an empty value.
    std::unordered_map<std::string, std::string> m_arguments;

    // Convert parameter names to lowercase so that:
    //
    // -CreateProfile
    // -createprofile
    // -CREATEPROFILE
    //
    // are treated the same.
    static std::string normalize(
        const std::string& name
    );
};