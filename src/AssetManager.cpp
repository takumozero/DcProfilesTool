#include "AssetManager.h"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    constexpr const char* FFMPEG_DOWNLOAD_URL =
        "https://github.com/BtbN/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-win64-gpl.zip";

    constexpr const char* FFMPEG_ARCHIVE_NAME =
        "ffmpeg_download.zip";

    // Quote paths for CreateProcess using the Windows command-line rules.
    std::wstring quoteWindowsArgument(const std::wstring& value)
    {
        std::wstring quoted = L"\"";
        std::size_t backslashes = 0;

        for (const wchar_t character : value)
        {
            if (character == L'\\')
            {
                ++backslashes;
                continue;
            }

            if (character == L'\"')
            {
                quoted.append(backslashes * 2 + 1, L'\\');
                quoted += character;
                backslashes = 0;
                continue;
            }

            quoted.append(backslashes, L'\\');
            backslashes = 0;
            quoted += character;
        }

        quoted.append(backslashes * 2, L'\\');
        quoted += L'\"';
        return quoted;
    }
}

namespace
{
    constexpr const char* GIFSICLE_DOWNLOAD_URL =
        "https://eternallybored.org/misc/gifsicle/releases/gifsicle-1.95-win64.zip";

    constexpr const char* GIFSICLE_ARCHIVE_NAME =
        "gifsicle_download.zip";
}

bool AssetManager::askToDownloadGifsicle() const
{
    std::cout
        << "\nGifsicle was not found.\n"
        << "Gifsicle is required to optimize oversized GIF files.\n\n"
        << "Download Gifsicle into the program directory? [y/N]: ";

    std::string answer;

    std::getline(
        std::cin,
        answer
    );

    return answer == "y" ||
        answer == "Y";
}

void AssetManager::downloadGifsicle() const
{
    const auto archivePath =
        m_programDirectory /
        GIFSICLE_ARCHIVE_NAME;

    const auto temporaryDirectory =
        m_programDirectory /
        "gifsicle_temp";

    std::cout
        << "\nDownloading Gifsicle...\n";

    const std::string downloadCommand =
        "curl.exe "
        "--fail "
        "--location "
        "--silent "
        "--show-error "
        "--output \"" +
        archivePath.string() +
        "\" \"" +
        GIFSICLE_DOWNLOAD_URL +
        "\"";

    const int downloadResult =
        std::system(
            downloadCommand.c_str()
        );

    if (downloadResult != 0)
    {
        std::filesystem::remove(
            archivePath
        );

        throw std::runtime_error(
            "Failed to download Gifsicle."
        );
    }

    std::cout
        << "Download completed.\n"
        << "Extracting Gifsicle...\n";

    std::filesystem::remove_all(
        temporaryDirectory
    );

    std::filesystem::create_directories(
        temporaryDirectory
    );

    const std::string extractCommand =
        "powershell.exe "
        "-NoProfile "
        "-NonInteractive "
        "-Command "
        "\"Expand-Archive "
        "-LiteralPath '" +
        archivePath.string() +
        "' "
        "-DestinationPath '" +
        temporaryDirectory.string() +
        "' "
        "-Force\"";

    const int extractResult =
        std::system(
            extractCommand.c_str()
        );

    if (extractResult != 0)
    {
        std::filesystem::remove(
            archivePath
        );

        std::filesystem::remove_all(
            temporaryDirectory
        );

        throw std::runtime_error(
            "Failed to extract Gifsicle."
        );
    }

    std::filesystem::path foundGifsicle;

    for (const auto& entry :
        std::filesystem::recursive_directory_iterator(
            temporaryDirectory))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        if (entry.path().filename() == "gifsicle.exe")
        {
            foundGifsicle =
                entry.path();

            break;
        }
    }

    if (foundGifsicle.empty())
    {
        std::filesystem::remove(
            archivePath
        );

        std::filesystem::remove_all(
            temporaryDirectory
        );

        throw std::runtime_error(
            "gifsicle.exe was not found "
            "inside the downloaded archive."
        );
    }

    const auto destination =
        m_programDirectory /
        "gifsicle.exe";

    std::filesystem::copy_file(
        foundGifsicle,
        destination,
        std::filesystem::copy_options::overwrite_existing
    );

    std::filesystem::remove(
        archivePath
    );

    std::filesystem::remove_all(
        temporaryDirectory
    );

    if (!std::filesystem::exists(destination))
    {
        throw std::runtime_error(
            "Gifsicle installation failed."
        );
    }

    std::cout
        << "Gifsicle installed successfully:\n"
        << destination
        << "\n";
}

std::filesystem::path AssetManager::ensureGifsicle()
{
    const auto existingPath =
        getGifsiclePath();

    if (!existingPath.empty())
    {
        return existingPath;
    }

    if (!askToDownloadGifsicle())
    {
        throw std::runtime_error(
            "Gifsicle is required to optimize "
            "oversized GIF files, but was not installed. "
            "Operation aborted."
        );
    }

    downloadGifsicle();

    const auto installedPath =
        m_programDirectory /
        "gifsicle.exe";

    if (!std::filesystem::exists(installedPath))
    {
        throw std::runtime_error(
            "Gifsicle installation could not be verified."
        );
    }

    return installedPath;
}

AssetManager::AssetManager(
    const std::filesystem::path& programDirectory)
    : m_programDirectory(
        std::filesystem::absolute(programDirectory))
{
}

std::uintmax_t AssetManager::getFileSize(
    const std::filesystem::path& file) const
{
    if (!std::filesystem::exists(file))
    {
        throw std::runtime_error(
            "Asset does not exist: " +
            file.string()
        );
    }

    if (!std::filesystem::is_regular_file(file))
    {
        throw std::runtime_error(
            "Asset is not a file: " +
            file.string()
        );
    }

    return std::filesystem::file_size(file);
}

bool AssetManager::isAvatarWithinLimit(
    const std::filesystem::path& file) const
{
    return getFileSize(file) <= AVATAR_MAX_SIZE;
}

bool AssetManager::isBannerWithinLimit(
    const std::filesystem::path& file) const
{
    return getFileSize(file) <= BANNER_MAX_SIZE;
}

std::filesystem::path AssetManager::findFfmpegInPath() const
{
#ifdef _WIN32

    char* pathEnvironment = nullptr;
    std::size_t requiredSize = 0;

    if (_dupenv_s(
        &pathEnvironment,
        &requiredSize,
        "PATH") != 0 ||
        pathEnvironment == nullptr)
    {
        return {};
    }

    std::string pathVariable =
        pathEnvironment;

    free(pathEnvironment);

#else

    const char* pathEnvironment =
        std::getenv("PATH");

    if (pathEnvironment == nullptr)
    {
        return {};
    }

    std::string pathVariable =
        pathEnvironment;

#endif

#ifdef _WIN32
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif

    std::size_t start = 0;

    while (start <= pathVariable.length())
    {
        const std::size_t end =
            pathVariable.find(separator, start);

        const std::string directory =
            pathVariable.substr(
                start,
                end == std::string::npos
                ? std::string::npos
                : end - start
            );

        if (!directory.empty())
        {
#ifdef _WIN32
            const auto ffmpegPath =
                std::filesystem::path(directory) /
                "ffmpeg.exe";
#else
            const auto ffmpegPath =
                std::filesystem::path(directory) /
                "ffmpeg";
#endif

            if (std::filesystem::exists(ffmpegPath) &&
                std::filesystem::is_regular_file(ffmpegPath))
            {
                return ffmpegPath;
            }
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return {};
}

std::filesystem::path AssetManager::findFfprobeInPath() const
{
#ifdef _WIN32

    char* pathEnvironment = nullptr;
    std::size_t requiredSize = 0;

    if (_dupenv_s(
        &pathEnvironment,
        &requiredSize,
        "PATH") != 0 ||
        pathEnvironment == nullptr)
    {
        return {};
    }

    std::string pathVariable =
        pathEnvironment;

    free(pathEnvironment);

#else

    const char* pathEnvironment =
        std::getenv("PATH");

    if (pathEnvironment == nullptr)
    {
        return {};
    }

    std::string pathVariable =
        pathEnvironment;

#endif

#ifdef _WIN32
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif

    std::size_t start = 0;

    while (start <= pathVariable.length())
    {
        const std::size_t end =
            pathVariable.find(separator, start);

        const std::string directory =
            pathVariable.substr(
                start,
                end == std::string::npos
                ? std::string::npos
                : end - start
            );

        if (!directory.empty())
        {
#ifdef _WIN32
            const auto ffprobePath =
                std::filesystem::path(directory) /
                "ffprobe.exe";
#else
            const auto ffprobePath =
                std::filesystem::path(directory) /
                "ffprobe";
#endif

            if (std::filesystem::exists(ffprobePath) &&
                std::filesystem::is_regular_file(ffprobePath))
            {
                return ffprobePath;
            }
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return {};
}

std::filesystem::path AssetManager::getFfprobePath() const
{
    const auto localFfprobe =
        m_programDirectory / "ffprobe.exe";

    if (std::filesystem::exists(localFfprobe) &&
        std::filesystem::is_regular_file(localFfprobe))
    {
        return localFfprobe;
    }

    return findFfprobeInPath();
}

double AssetManager::getGifFps(
    const std::filesystem::path& file
) const
{
#ifdef _WIN32

    const auto ffprobe =
        getFfprobePath();

    if (ffprobe.empty())
    {
        throw std::runtime_error(
            "FFprobe was not found. "
            "FFprobe is required to determine "
            "the GIF frame rate."
        );
    }

    if (!std::filesystem::exists(file))
    {
        throw std::runtime_error(
            "GIF file does not exist: " +
            file.string()
        );
    }

    /*
     * Ask FFprobe for the average frame rate
     * of the first video stream.
     *
     * Example output:
     *
     *     60/1
     *     25/1
     *     30000/1001
     */
    std::wstring commandLine =
        quoteWindowsArgument(ffprobe.wstring()) +
        L" "
        L"-v error "
        L"-select_streams v:0 "
        L"-show_entries stream=avg_frame_rate "
        L"-of default=noprint_wrappers=1:nokey=1 " +
        quoteWindowsArgument(file.wstring());

    /*
     * Create a pipe so that we can capture
     * FFprobe's stdout.
     */
    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength =
        sizeof(SECURITY_ATTRIBUTES);

    securityAttributes.bInheritHandle =
        TRUE;

    securityAttributes.lpSecurityDescriptor =
        nullptr;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;

    if (!CreatePipe(
        &readPipe,
        &writePipe,
        &securityAttributes,
        0))
    {
        throw std::runtime_error(
            "Failed to create pipe for FFprobe."
        );
    }

    /*
     * The read side must not be inherited
     * by the child process.
     */
    if (!SetHandleInformation(
        readPipe,
        HANDLE_FLAG_INHERIT,
        0))
    {
        CloseHandle(readPipe);
        CloseHandle(writePipe);

        throw std::runtime_error(
            "Failed to configure FFprobe output pipe."
        );
    }

    std::vector<wchar_t> commandBuffer(
        commandLine.begin(),
        commandLine.end()
    );

    commandBuffer.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb =
        sizeof(startupInfo);

    startupInfo.dwFlags =
        STARTF_USESTDHANDLES;

    startupInfo.hStdOutput =
        writePipe;

    startupInfo.hStdError =
        writePipe;

    PROCESS_INFORMATION processInfo{};

    const BOOL started =
        CreateProcessW(
            nullptr,
            commandBuffer.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

    /*
     * The parent no longer needs the write side.
     */
    CloseHandle(writePipe);

    if (!started)
    {
        CloseHandle(readPipe);

        const DWORD errorCode =
            GetLastError();

        throw std::runtime_error(
            "Failed to start FFprobe. "
            "Windows error code: " +
            std::to_string(errorCode)
        );
    }

    /*
     * Read FFprobe's output.
     */
    std::string output;

    char buffer[256];

    DWORD bytesRead = 0;

    while (true)
    {
        const BOOL success =
            ReadFile(
                readPipe,
                buffer,
                sizeof(buffer) - 1,
                &bytesRead,
                nullptr
            );

        if (!success || bytesRead == 0)
        {
            break;
        }

        buffer[bytesRead] =
            '\0';

        output += buffer;
    }

    CloseHandle(readPipe);

    /*
     * Wait until FFprobe has finished.
     */
    WaitForSingleObject(
        processInfo.hProcess,
        INFINITE
    );

    DWORD exitCode = 0;

    GetExitCodeProcess(
        processInfo.hProcess,
        &exitCode
    );

    CloseHandle(
        processInfo.hProcess
    );

    CloseHandle(
        processInfo.hThread
    );

    if (exitCode != 0)
    {
        throw std::runtime_error(
            "FFprobe failed while determining "
            "GIF FPS.\n"
            "FFprobe output: " +
            output
        );
    }

    /*
     * Remove whitespace around the result.
     */
    while (!output.empty() &&
        std::isspace(
            static_cast<unsigned char>(
                output.back())))
    {
        output.pop_back();
    }

    std::size_t first =
        0;

    while (first < output.size() &&
        std::isspace(
            static_cast<unsigned char>(
                output[first])))
    {
        ++first;
    }

    output =
        output.substr(first);

    if (output.empty())
    {
        throw std::runtime_error(
            "FFprobe did not return a frame rate "
            "for GIF: " +
            file.string()
        );
    }

    /*
     * Parse the rational number.
     *
     * Examples:
     *
     *     60/1
     *     25/1
     *     30000/1001
     */
    const std::size_t slash =
        output.find('/');

    try
    {
        double fps = 0.0;

        if (slash != std::string::npos)
        {
            const double numerator =
                std::stod(
                    output.substr(
                        0,
                        slash
                    )
                );

            const double denominator =
                std::stod(
                    output.substr(
                        slash + 1
                    )
                );

            if (denominator == 0.0)
            {
                throw std::runtime_error(
                    "FFprobe returned an invalid "
                    "frame rate with denominator 0."
                );
            }

            fps =
                numerator /
                denominator;
        }
        else
        {
            fps =
                std::stod(output);
        }

        if (fps <= 0.0)
        {
            throw std::runtime_error(
                "FFprobe returned an invalid "
                "frame rate: " +
                output
            );
        }

        return fps;
    }
    catch (const std::exception&)
    {
        throw std::runtime_error(
            "Failed to parse GIF frame rate "
            "returned by FFprobe: " +
            output
        );
    }

#else

    throw std::runtime_error(
        "FFprobe processing is currently "
        "only implemented on Windows."
    );

#endif
}

std::filesystem::path AssetManager::getFfmpegPath() const
{
    const auto localFfmpeg =
        m_programDirectory / "ffmpeg.exe";

    if (std::filesystem::exists(localFfmpeg) &&
        std::filesystem::is_regular_file(localFfmpeg))
    {
        return localFfmpeg;
    }

    return findFfmpegInPath();
}

bool AssetManager::isFfmpegAvailable() const
{
    return !getFfmpegPath().empty();
}

bool AssetManager::askToDownloadFfmpeg() const
{
    std::cout
        << "\nFFmpeg was not found.\n"
        << "FFmpeg is required to process profile assets "
        << "that exceed Discord's file-size limits.\n\n"
        << "Download FFmpeg into the program directory? [y/N]: ";

    std::string answer;

    std::getline(
        std::cin,
        answer
    );

    return answer == "y" ||
        answer == "Y";
}

void AssetManager::downloadFfmpeg() const
{
    const auto archivePath =
        m_programDirectory /
        FFMPEG_ARCHIVE_NAME;

    const auto temporaryDirectory =
        m_programDirectory /
        "ffmpeg_temp";

    std::cout
        << "\nDownloading FFmpeg...\n";

    const std::string downloadCommand =
        "curl.exe "
        "--fail "
        "--location "
        "--silent "
        "--show-error "
        "--output \"" +
        archivePath.string() +
        "\" \"" +
        FFMPEG_DOWNLOAD_URL +
        "\"";

    const int downloadResult =
        std::system(
            downloadCommand.c_str()
        );

    if (downloadResult != 0)
    {
        std::filesystem::remove(
            archivePath
        );

        throw std::runtime_error(
            "Failed to download FFmpeg."
        );
    }

    std::cout
        << "Download completed.\n"
        << "Extracting FFmpeg...\n";

    std::filesystem::remove_all(
        temporaryDirectory
    );

    std::filesystem::create_directories(
        temporaryDirectory
    );

    const std::string extractCommand =
        "powershell.exe "
        "-NoProfile "
        "-NonInteractive "
        "-Command "
        "\"Expand-Archive "
        "-LiteralPath '" +
        archivePath.string() +
        "' "
        "-DestinationPath '" +
        temporaryDirectory.string() +
        "' "
        "-Force\"";

    const int extractResult =
        std::system(
            extractCommand.c_str()
        );

    if (extractResult != 0)
    {
        std::filesystem::remove(
            archivePath
        );

        std::filesystem::remove_all(
            temporaryDirectory
        );

        throw std::runtime_error(
            "Failed to extract FFmpeg."
        );
    }

    /*
     * Find both FFmpeg and FFprobe inside
     * the downloaded archive.
     */
    std::filesystem::path foundFfmpeg;
    std::filesystem::path foundFfprobe;

    for (const auto& entry :
        std::filesystem::recursive_directory_iterator(
            temporaryDirectory))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const auto filename =
            entry.path().filename();

        if (filename == "ffmpeg.exe")
        {
            foundFfmpeg =
                entry.path();
        }
        else if (filename == "ffprobe.exe")
        {
            foundFfprobe =
                entry.path();
        }

        /*
         * We have found both files, so there is
         * no reason to continue searching.
         */
        if (!foundFfmpeg.empty() &&
            !foundFfprobe.empty())
        {
            break;
        }
    }

    /*
     * Verify FFmpeg was found.
     */
    if (foundFfmpeg.empty())
    {
        std::filesystem::remove(
            archivePath
        );

        std::filesystem::remove_all(
            temporaryDirectory
        );

        throw std::runtime_error(
            "FFmpeg executable was not found "
            "inside the downloaded archive."
        );
    }

    /*
     * Verify FFprobe was found.
     */
    if (foundFfprobe.empty())
    {
        std::filesystem::remove(
            archivePath
        );

        std::filesystem::remove_all(
            temporaryDirectory
        );

        throw std::runtime_error(
            "FFprobe executable was not found "
            "inside the downloaded FFmpeg archive."
        );
    }

    /*
     * Install FFmpeg.
     */
    const auto ffmpegDestination =
        m_programDirectory /
        "ffmpeg.exe";

    std::filesystem::copy_file(
        foundFfmpeg,
        ffmpegDestination,
        std::filesystem::copy_options::overwrite_existing
    );

    /*
     * Install FFprobe.
     */
    const auto ffprobeDestination =
        m_programDirectory /
        "ffprobe.exe";

    std::filesystem::copy_file(
        foundFfprobe,
        ffprobeDestination,
        std::filesystem::copy_options::overwrite_existing
    );

    /*
     * Clean up downloaded archive and
     * temporary extraction directory.
     */
    std::filesystem::remove(
        archivePath
    );

    std::filesystem::remove_all(
        temporaryDirectory
    );

    /*
     * Verify both installations.
     */
    if (!std::filesystem::exists(
        ffmpegDestination))
    {
        throw std::runtime_error(
            "FFmpeg installation failed."
        );
    }

    if (!std::filesystem::exists(
        ffprobeDestination))
    {
        throw std::runtime_error(
            "FFprobe installation failed."
        );
    }

    std::cout
        << "FFmpeg installed successfully:\n"
        << ffmpegDestination
        << "\n";

    std::cout
        << "FFprobe installed successfully:\n"
        << ffprobeDestination
        << "\n";
}

std::filesystem::path AssetManager::ensureFfmpeg()
{
    const auto existingPath =
        getFfmpegPath();

    if (!existingPath.empty())
    {
        return existingPath;
    }

    if (!askToDownloadFfmpeg())
    {
        throw std::runtime_error(
            "FFmpeg is required, but was not installed. "
            "Operation aborted."
        );
    }

    downloadFfmpeg();

    const auto installedPath =
        m_programDirectory /
        "ffmpeg.exe";

    if (!std::filesystem::exists(installedPath))
    {
        throw std::runtime_error(
            "FFmpeg installation could not be verified."
        );
    }

    return installedPath;
}

std::string AssetManager::getExtension(
    const std::filesystem::path& file)
{
    std::string extension =
        file.extension().string();

    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(
                std::tolower(c)
                );
        }
    );

    return extension;
}

void AssetManager::processAvatar(
    const std::filesystem::path& source,
    const std::filesystem::path& destination)
{
    processImage(
        source,
        destination,
        AVATAR_MAX_SIZE,
        "avatar"
    );
}

void AssetManager::processBanner(
    const std::filesystem::path& source,
    const std::filesystem::path& destination)
{
    processImage(
        source,
        destination,
        BANNER_MAX_SIZE,
        "banner"
    );
}

void AssetManager::processImage(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::uintmax_t maximumSize,
    const std::string& assetType)
{
    if (!std::filesystem::exists(source))
    {
        throw std::runtime_error(
            "Source " + assetType +
            " does not exist: " +
            source.string()
        );
    }

    if (!std::filesystem::is_regular_file(source))
    {
        throw std::runtime_error(
            "Source " + assetType +
            " is not a file: " +
            source.string()
        );
    }

    const auto extension =
        getExtension(source);

    if (extension != ".png" &&
        extension != ".jpg" &&
        extension != ".jpeg" &&
        extension != ".gif" &&
        extension != ".webp")
    {
        throw std::runtime_error(
            "Unsupported " + assetType +
            " image format: " +
            extension
        );
    }

    const auto sourceSize =
        getFileSize(source);

    std::filesystem::create_directories(
        destination.parent_path()
    );

    /*
     * If the original is already within the
     * allowed size, simply copy it.
     *
     * The original source is never modified.
     */
    if (sourceSize <= maximumSize)
    {
        std::filesystem::copy_file(
            source,
            destination,
            std::filesystem::copy_options::overwrite_existing
        );

        return;
    }

    /*
     * GIF files have their own optimization
     * pipeline using Gifsicle.
     *
     * Resolution and FPS are preserved during
     * the lossless and lossy compression stages.
     */
    if (extension == ".gif")
    {
        /*
         * -----------------------------------------
         * GIF processing
         * -----------------------------------------
         *
         * Quality reduction order:
         *
         *   30 FPS
         *      ↓
         *   128 colors
         *      ↓
         *   lossy 10 → 50
         *      ↓
         *   64 colors
         *      ↓
         *   lossy 10 → 50
         *
         * If the GIF is still too large:
         *
         *   24 FPS
         *      ↓
         *   same pipeline
         *
         * If still too large:
         *
         *   20 FPS
         *      ↓
         *   same pipeline
         *
         * Resolution is never changed.
         */

        const double sourceFps =
            getGifFps(source);

        std::cout
            << "\nSource FPS: "
            << sourceFps
            << "\n";

        /*
         * Determine the first FPS level.
         *
         * GIFs above 30 FPS start at 30.
         * GIFs below 30 FPS start at their
         * original FPS.
         */
        int startingFps = 0;

        if (sourceFps > 30.0)
        {
            startingFps = 30;
        }
        else
        {
            startingFps =
                static_cast<int>(
                    std::ceil(sourceFps)
                    );
        }

        /*
         * Build the FPS fallback list.
         *
         * Examples:
         *
         * 60 FPS → 30, 24, 20
         * 25 FPS → 25, 24, 20
         * 24 FPS → 24, 20
         * 20 FPS → 20
         * 15 FPS → 15
         */
        std::vector<int> fpsLevels;

        fpsLevels.push_back(
            startingFps
        );

        if (startingFps > 24)
        {
            fpsLevels.push_back(24);
        }

        if (startingFps > 20)
        {
            fpsLevels.push_back(20);
        }

        for (const int fps :
        fpsLevels)
        {
            std::cout
                << "\nProcessing "
                << assetType
                << " at "
                << fps
                << " FPS...\n";

            if (processGifAtFps(
                source,
                destination,
                maximumSize,
                fps,
                sourceFps))
            {
                std::cout
                    << assetType
                    << " successfully processed at "
                    << fps
                    << " FPS.\n";

                return;
            }
        }

        /*
         * None of the supported FPS levels were
         * able to bring the GIF below the limit.
         */
        throw std::runtime_error(
            "The " + assetType +
            " is still too large after GIF "
            "optimization.\n"
            "Tried FPS levels: 30, 24 and 20.\n"
            "Minimum FPS: " +
            std::to_string(GIF_MIN_FPS) +
            "\n"
            "Minimum color count: 64\n"
            "Maximum lossy compression level: " +
            std::to_string(
                GIF_MAX_LOSSY_LEVEL
            )
        );
    }

    /*
     * -----------------------------------------
     * Non-GIF images
     * -----------------------------------------
     *
     * PNG and WebP use the FFmpeg lossless
     * compression pipeline.
     */
    const auto temporaryOutput =
        destination.parent_path() /
        (destination.stem().string() +
            "_compressed" +
            destination.extension().string());

    try
    {
        if (std::filesystem::exists(
            temporaryOutput))
        {
            std::filesystem::remove(
                temporaryOutput
            );
        }

        compressLossless(
            source,
            temporaryOutput
        );

        if (!std::filesystem::exists(
            temporaryOutput))
        {
            throw std::runtime_error(
                "FFmpeg did not create the "
                "compressed " + assetType + "."
            );
        }

        const auto compressedSize =
            getFileSize(
                temporaryOutput
            );

        if (compressedSize > maximumSize)
        {
            std::filesystem::remove(
                temporaryOutput
            );

            throw std::runtime_error(
                "The " + assetType +
                " is still too large after "
                "lossless compression.\n"
                "Original size: " +
                std::to_string(
                    static_cast<double>(sourceSize) /
                    (1024.0 * 1024.0)
                ) +
                " MiB\n"
                "Compressed size: " +
                std::to_string(
                    static_cast<double>(compressedSize) /
                    (1024.0 * 1024.0)
                ) +
                " MiB\n"
                "Maximum size: " +
                std::to_string(
                    static_cast<double>(maximumSize) /
                    (1024.0 * 1024.0)
                ) +
                " MiB"
            );
        }

        /*
         * Only now do we create the managed
         * asset.
         */
        std::filesystem::copy_file(
            temporaryOutput,
            destination,
            std::filesystem::copy_options::overwrite_existing
        );

        std::filesystem::remove(
            temporaryOutput
        );
    }
    catch (...)
    {
        if (std::filesystem::exists(
            temporaryOutput))
        {
            std::filesystem::remove(
                temporaryOutput
            );
        }

        throw;
    }
}

void AssetManager::compressLossless(
    const std::filesystem::path& input,
    const std::filesystem::path& output)
{
#ifdef _WIN32

    const auto ffmpeg =
        ensureFfmpeg();

    const auto extension =
        getExtension(input);

    /*
     * JPEG cannot be compressed losslessly.
     *
     * GIF is intentionally not handled here.
     * GIF files are optimized separately using Gifsicle.
     */
    if (extension == ".jpg" ||
        extension == ".jpeg")
    {
        throw std::runtime_error(
            "JPEG images cannot be reduced "
            "losslessly. The image exceeds "
            "the allowed size."
        );
    }

    if (extension == ".gif")
    {
        throw std::runtime_error(
            "GIF files must be processed using "
            "Gifsicle, not FFmpeg."
        );
    }

    if (extension != ".png" &&
        extension != ".webp")
    {
        throw std::runtime_error(
            "Unsupported image format for "
            "lossless compression: " +
            extension
        );
    }

    std::wstring arguments;

    /*
     * Input file.
     */
    arguments =
        L"-y "
        L"-i " +
        quoteWindowsArgument(input.wstring()) +
        L" ";

    /*
     * PNG
     *
     * compression_level 9 gives FFmpeg's highest
     * PNG compression level.
     *
     * -update 1 and -frames:v 1 tell FFmpeg that
     * this is a single output image rather than
     * an image sequence.
     */
    if (extension == ".png")
    {
        arguments +=
            L"-c:v png "
            L"-compression_level 9 "
            L"-update 1 "
            L"-frames:v 1 ";
    }

    /*
     * WebP
     *
     * Use WebP's lossless mode.
     */
    else if (extension == ".webp")
    {
        arguments +=
            L"-c:v libwebp "
            L"-lossless 1 ";
    }

    /*
     * Output file.
     */
    arguments +=
        quoteWindowsArgument(output.wstring());

    /*
     * CreateProcess requires the executable
     * and arguments in one command line.
     *
     * The executable path is quoted so paths
     * such as:
     *
     * C:\Program Files\...
     *
     * work correctly.
     */
    std::wstring commandLine =
        quoteWindowsArgument(ffmpeg.wstring()) +
        L" " +
        arguments;

    /*
     * CreateProcess may modify the command-line
     * buffer, so it must receive a writable buffer.
     */
    std::vector<wchar_t> commandBuffer(
        commandLine.begin(),
        commandLine.end()
    );

    commandBuffer.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb =
        sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};

    const BOOL started =
        CreateProcessW(
            nullptr,
            commandBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

    if (!started)
    {
        const DWORD errorCode =
            GetLastError();

        throw std::runtime_error(
            "Failed to start FFmpeg. "
            "Windows error code: " +
            std::to_string(errorCode)
        );
    }

    /*
     * Wait until FFmpeg finishes.
     */
    WaitForSingleObject(
        processInfo.hProcess,
        INFINITE
    );

    DWORD exitCode = 0;

    GetExitCodeProcess(
        processInfo.hProcess,
        &exitCode
    );

    CloseHandle(
        processInfo.hProcess
    );

    CloseHandle(
        processInfo.hThread
    );

    if (exitCode != 0)
    {
        throw std::runtime_error(
            "FFmpeg failed while compressing: " +
            input.string() +
            "\nFFmpeg exit code: " +
            std::to_string(exitCode)
        );
    }

#else

    throw std::runtime_error(
        "FFmpeg asset processing is currently "
        "only implemented on Windows."
    );

#endif
}

std::filesystem::path AssetManager::findGifsicleInPath() const
{
#ifdef _WIN32

    char* pathEnvironment = nullptr;
    std::size_t requiredSize = 0;

    if (_dupenv_s(
        &pathEnvironment,
        &requiredSize,
        "PATH") != 0 ||
        pathEnvironment == nullptr)
    {
        return {};
    }

    std::string pathVariable =
        pathEnvironment;

    free(pathEnvironment);

#else

    const char* pathEnvironment =
        std::getenv("PATH");

    if (pathEnvironment == nullptr)
    {
        return {};
    }

    std::string pathVariable =
        pathEnvironment;

#endif

#ifdef _WIN32
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif

    std::size_t start = 0;

    while (start <= pathVariable.length())
    {
        const std::size_t end =
            pathVariable.find(separator, start);

        const std::string directory =
            pathVariable.substr(
                start,
                end == std::string::npos
                ? std::string::npos
                : end - start
            );

        if (!directory.empty())
        {
#ifdef _WIN32
            const auto gifsiclePath =
                std::filesystem::path(directory) /
                "gifsicle.exe";
#else
            const auto gifsiclePath =
                std::filesystem::path(directory) /
                "gifsicle";
#endif

            if (std::filesystem::exists(gifsiclePath) &&
                std::filesystem::is_regular_file(gifsiclePath))
            {
                return gifsiclePath;
            }
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return {};
}

std::filesystem::path AssetManager::getGifsiclePath() const
{
    const auto localGifsicle =
        m_programDirectory / "gifsicle.exe";

    if (std::filesystem::exists(localGifsicle) &&
        std::filesystem::is_regular_file(localGifsicle))
    {
        return localGifsicle;
    }

    return findGifsicleInPath();
}

bool AssetManager::isGifsicleAvailable() const
{
    return !getGifsiclePath().empty();
}

void AssetManager::optimizeGif(
    const std::filesystem::path& input,
    const std::filesystem::path& output)
{
#ifdef _WIN32

    const auto gifsicle =
        ensureGifsicle();

    std::wstring commandLine =
        quoteWindowsArgument(gifsicle.wstring()) +
        L" "
        L"--optimize=3 " +
        quoteWindowsArgument(input.wstring()) +
        L" "
        L"--output " +
        quoteWindowsArgument(output.wstring());

    std::vector<wchar_t> commandBuffer(
        commandLine.begin(),
        commandLine.end()
    );

    commandBuffer.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb =
        sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};

    const BOOL started =
        CreateProcessW(
            nullptr,
            commandBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

    if (!started)
    {
        const DWORD errorCode =
            GetLastError();

        throw std::runtime_error(
            "Failed to start Gifsicle. "
            "Windows error code: " +
            std::to_string(errorCode)
        );
    }

    WaitForSingleObject(
        processInfo.hProcess,
        INFINITE
    );

    DWORD exitCode = 0;

    GetExitCodeProcess(
        processInfo.hProcess,
        &exitCode
    );

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    if (exitCode != 0)
    {
        throw std::runtime_error(
            "Gifsicle failed while optimizing GIF. "
            "Exit code: " +
            std::to_string(exitCode)
        );
    }

#else

    throw std::runtime_error(
        "Gifsicle processing is currently "
        "only implemented on Windows."
    );

#endif
}

void AssetManager::optimizeGifLossy(
    const std::filesystem::path& input,
    const std::filesystem::path& output,
    int lossyLevel)
{
#ifdef _WIN32

    const auto gifsicle =
        ensureGifsicle();

    std::wstring commandLine =
        quoteWindowsArgument(gifsicle.wstring()) +
        L" "
        L"--optimize=3 "
        L"--lossy=" +
        std::to_wstring(lossyLevel) +
        L" " +
        quoteWindowsArgument(input.wstring()) +
        L" "
        L"--output " +
        quoteWindowsArgument(output.wstring());

    std::vector<wchar_t> commandBuffer(
        commandLine.begin(),
        commandLine.end()
    );

    commandBuffer.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb =
        sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};

    const BOOL started =
        CreateProcessW(
            nullptr,
            commandBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

    if (!started)
    {
        throw std::runtime_error(
            "Failed to start Gifsicle."
        );
    }

    WaitForSingleObject(
        processInfo.hProcess,
        INFINITE
    );

    DWORD exitCode = 0;

    GetExitCodeProcess(
        processInfo.hProcess,
        &exitCode
    );

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    if (exitCode != 0)
    {
        throw std::runtime_error(
            "Gifsicle lossy compression failed. "
            "Exit code: " +
            std::to_string(exitCode)
        );
    }

#else

    throw std::runtime_error(
        "Gifsicle processing is currently "
        "only implemented on Windows."
    );

#endif
}

void AssetManager::reduceGifColors(
    const std::filesystem::path& input,
    const std::filesystem::path& output,
    int colors)
{
#ifdef _WIN32

    if (colors < 2 || colors > 256)
    {
        throw std::runtime_error(
            "Invalid GIF color count: " +
            std::to_string(colors)
        );
    }

    const auto gifsicle =
        ensureGifsicle();

    std::wstring commandLine =
        quoteWindowsArgument(gifsicle.wstring()) +
        L" "
        L"--optimize=3 "
        L"--colors " +
        std::to_wstring(colors) +
        L" " +
        quoteWindowsArgument(input.wstring()) +
        L" "
        L"--output " +
        quoteWindowsArgument(output.wstring());

    std::vector<wchar_t> commandBuffer(
        commandLine.begin(),
        commandLine.end()
    );

    commandBuffer.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb =
        sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};

    const BOOL started =
        CreateProcessW(
            nullptr,
            commandBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

    if (!started)
    {
        const DWORD errorCode =
            GetLastError();

        throw std::runtime_error(
            "Failed to start Gifsicle. "
            "Windows error code: " +
            std::to_string(errorCode)
        );
    }

    WaitForSingleObject(
        processInfo.hProcess,
        INFINITE
    );

    DWORD exitCode = 0;

    GetExitCodeProcess(
        processInfo.hProcess,
        &exitCode
    );

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    if (exitCode != 0)
    {
        throw std::runtime_error(
            "Gifsicle failed while reducing "
            "GIF colors to " +
            std::to_string(colors) +
            ". Exit code: " +
            std::to_string(exitCode)
        );
    }

#else

    throw std::runtime_error(
        "Gifsicle processing is currently "
        "only implemented on Windows."
    );

#endif
}

void AssetManager::reduceGifFps(
    const std::filesystem::path& input,
    const std::filesystem::path& output,
    int targetFps)
{
#ifdef _WIN32

    if (targetFps <= 0)
    {
        throw std::runtime_error(
            "Invalid target FPS: " +
            std::to_string(targetFps)
        );
    }

    const auto ffmpeg =
        ensureFfmpeg();

    /*
     * The fps filter drops frames to reach
     * the requested frame rate.
     *
     * No scaling is performed, so the
     * original GIF resolution is preserved.
     */
    const std::wstring filter =
        L"fps=" +
        std::to_wstring(targetFps);

    std::wstring commandLine =
        quoteWindowsArgument(ffmpeg.wstring()) +
        L" "
        L"-y "
        L"-i " +
        quoteWindowsArgument(input.wstring()) +
        L" "
        L"-vf \"" +
        filter +
        L"\" "
        L"-gifflags -offsetting " +
        quoteWindowsArgument(output.wstring());

    std::vector<wchar_t> commandBuffer(
        commandLine.begin(),
        commandLine.end()
    );

    commandBuffer.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb =
        sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};

    const BOOL started =
        CreateProcessW(
            nullptr,
            commandBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

    if (!started)
    {
        const DWORD errorCode =
            GetLastError();

        throw std::runtime_error(
            "Failed to start FFmpeg while "
            "reducing GIF FPS. "
            "Windows error code: " +
            std::to_string(errorCode)
        );
    }

    WaitForSingleObject(
        processInfo.hProcess,
        INFINITE
    );

    DWORD exitCode = 0;

    GetExitCodeProcess(
        processInfo.hProcess,
        &exitCode
    );

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    if (exitCode != 0)
    {
        throw std::runtime_error(
            "FFmpeg failed while reducing "
            "GIF FPS to " +
            std::to_string(targetFps) +
            ". Exit code: " +
            std::to_string(exitCode)
        );
    }

#else

    throw std::runtime_error(
        "FFmpeg processing is currently "
        "only implemented on Windows."
    );

#endif
}

bool AssetManager::processGifAtFps(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::uintmax_t maximumSize,
    int fps,
    double sourceFps)
{
    /*
     * This function tries to create a GIF that fits
     * within maximumSize while keeping the specified FPS.
     *
     * Quality reduction order:
     *
     *   FPS
     *       ↓
     *   Lossless optimization
     *       ↓
     *   128 colors
     *       ↓
     *   lossy 10 → 50
     *       ↓
     *   64 colors
     *       ↓
     *   lossy 10 → 50
     *
     * Resolution is never changed.
     */

    const auto temporaryDirectory =
        destination.parent_path() /
        (destination.stem().string() +
            "_gif_" +
            std::to_string(fps) +
            "fps_temp");

    std::filesystem::remove_all(
        temporaryDirectory
    );

    std::filesystem::create_directories(
        temporaryDirectory
    );

    /*
     * Helper for displaying file sizes.
     */
    const auto printSize =
        [&](const std::string& stage,
            const std::filesystem::path& file)
        {
            const auto size =
                getFileSize(file);

            const double sizeMiB =
                static_cast<double>(size) /
                (1024.0 * 1024.0);

            std::cout
                << "  "
                << std::left
                << std::setw(30)
                << stage
                << std::right
                << std::fixed
                << std::setprecision(2)
                << sizeMiB
                << " MiB";

            if (size <= maximumSize)
            {
                std::cout
                    << "  [OK]";
            }

            std::cout
                << "\n";
        };

    try
    {
        /*
         * -----------------------------------------
         * 1. FPS
         * -----------------------------------------
         */

        const auto fpsGif =
            temporaryDirectory /
            "fps.gif";

        if (fps < sourceFps)
        {
            std::cout
                << "  Reducing FPS: "
                << sourceFps
                << " → "
                << fps
                << "\n";

            reduceGifFps(
                source,
                fpsGif,
                fps
            );
        }
        else
        {
            /*
             * Source FPS is already equal to or
             * below the requested FPS.
             *
             * Keep the original animation.
             */
            std::filesystem::copy_file(
                source,
                fpsGif,
                std::filesystem::copy_options::overwrite_existing
            );

            std::cout
                << "  Original FPS is already "
                << "within target range.\n";
        }

        if (!std::filesystem::exists(fpsGif))
        {
            throw std::runtime_error(
                "FPS processing did not create "
                "the expected GIF."
            );
        }

        printSize(
            "After FPS processing:",
            fpsGif
        );

        /*
         * -----------------------------------------
         * 2. Lossless optimization
         * -----------------------------------------
         */

        const auto losslessGif =
            temporaryDirectory /
            "lossless.gif";

        optimizeGif(
            fpsGif,
            losslessGif
        );

        if (!std::filesystem::exists(losslessGif))
        {
            throw std::runtime_error(
                "Gifsicle did not create the "
                "losslessly optimized GIF."
            );
        }

        printSize(
            "Lossless optimization:",
            losslessGif
        );

        if (getFileSize(losslessGif) <= maximumSize)
        {
            std::filesystem::copy_file(
                losslessGif,
                destination,
                std::filesystem::copy_options::overwrite_existing
            );

            std::filesystem::remove_all(
                temporaryDirectory
            );

            return true;
        }

        /*
         * =========================================
         * 128 COLORS
         * =========================================
         */

        const auto colors128 =
            temporaryDirectory /
            "colors_128.gif";

        reduceGifColors(
            losslessGif,
            colors128,
            128
        );

        if (!std::filesystem::exists(colors128))
        {
            throw std::runtime_error(
                "Gifsicle did not create the "
                "128-color GIF."
            );
        }

        printSize(
            "128 colors:",
            colors128
        );

        if (getFileSize(colors128) <= maximumSize)
        {
            std::filesystem::copy_file(
                colors128,
                destination,
                std::filesystem::copy_options::overwrite_existing
            );

            std::filesystem::remove_all(
                temporaryDirectory
            );

            return true;
        }

        /*
         * -----------------------------------------
         * 128 colors + lossy
         * -----------------------------------------
         */

        std::vector<int> lossyLevels;

        for (int lossyLevel = 10;
            lossyLevel <= GIF_MAX_LOSSY_LEVEL;
            lossyLevel += 10)
        {
            lossyLevels.push_back(
                lossyLevel
            );
        }

        for (const int lossyLevel :
        lossyLevels)
        {
            const auto lossyOutput =
                temporaryDirectory /
                ("colors_128_lossy_" +
                    std::to_string(lossyLevel) +
                    ".gif");

            if (std::filesystem::exists(
                lossyOutput))
            {
                std::filesystem::remove(
                    lossyOutput
                );
            }

            optimizeGifLossy(
                colors128,
                lossyOutput,
                lossyLevel
            );

            if (!std::filesystem::exists(
                lossyOutput))
            {
                continue;
            }

            printSize(
                "128 colors + lossy " +
                std::to_string(lossyLevel) +
                ":",
                lossyOutput
            );

            if (getFileSize(lossyOutput) <= maximumSize)
            {
                std::filesystem::copy_file(
                    lossyOutput,
                    destination,
                    std::filesystem::copy_options::overwrite_existing
                );

                std::filesystem::remove_all(
                    temporaryDirectory
                );

                return true;
            }

            std::filesystem::remove(
                lossyOutput
            );
        }

        /*
         * =========================================
         * 64 COLORS
         * =========================================
         */

        const auto colors64 =
            temporaryDirectory /
            "colors_64.gif";

        reduceGifColors(
            colors128,
            colors64,
            64
        );

        if (!std::filesystem::exists(colors64))
        {
            throw std::runtime_error(
                "Gifsicle did not create the "
                "64-color GIF."
            );
        }

        printSize(
            "64 colors:",
            colors64
        );

        if (getFileSize(colors64) <= maximumSize)
        {
            std::filesystem::copy_file(
                colors64,
                destination,
                std::filesystem::copy_options::overwrite_existing
            );

            std::filesystem::remove_all(
                temporaryDirectory
            );

            return true;
        }

        /*
         * -----------------------------------------
         * 64 colors + lossy
         * -----------------------------------------
         */

        for (const int lossyLevel :
        lossyLevels)
        {
            const auto lossyOutput =
                temporaryDirectory /
                ("colors_64_lossy_" +
                    std::to_string(lossyLevel) +
                    ".gif");

            if (std::filesystem::exists(
                lossyOutput))
            {
                std::filesystem::remove(
                    lossyOutput
                );
            }

            optimizeGifLossy(
                colors64,
                lossyOutput,
                lossyLevel
            );

            if (!std::filesystem::exists(
                lossyOutput))
            {
                continue;
            }

            printSize(
                "64 colors + lossy " +
                std::to_string(lossyLevel) +
                ":",
                lossyOutput
            );

            if (getFileSize(lossyOutput) <= maximumSize)
            {
                std::filesystem::copy_file(
                    lossyOutput,
                    destination,
                    std::filesystem::copy_options::overwrite_existing
                );

                std::filesystem::remove_all(
                    temporaryDirectory
                );

                return true;
            }

            std::filesystem::remove(
                lossyOutput
            );
        }

        /*
         * Nothing at this FPS was small enough.
         */
        std::filesystem::remove_all(
            temporaryDirectory
        );

        return false;
    }
    catch (...)
    {
        /*
         * Never leave temporary files behind
         * if something goes wrong.
         */
        std::filesystem::remove_all(
            temporaryDirectory
        );

        throw;
    }
}
