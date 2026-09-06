#pragma once

#include <filesystem>
#include <cstdint>
#include <string>


class AssetManager
{
public:
    static constexpr std::uintmax_t AVATAR_MAX_SIZE =
        10ULL * 1024ULL * 1024ULL;

    static constexpr std::uintmax_t BANNER_MAX_SIZE =
        8ULL * 1024ULL * 1024ULL;

    // GIF quality limits
    static constexpr int GIF_MAX_LOSSY_LEVEL = 80;
    static constexpr int GIF_MIN_FPS = 20;

    explicit AssetManager(
        const std::filesystem::path& programDirectory
    );

    bool isAvatarWithinLimit(
        const std::filesystem::path& file
    ) const;

    bool isBannerWithinLimit(
        const std::filesystem::path& file
    ) const;

    std::uintmax_t getFileSize(
        const std::filesystem::path& file
    ) const;

    bool isFfmpegAvailable() const;

    std::filesystem::path getFfmpegPath() const;

    std::filesystem::path ensureFfmpeg();

    void processAvatar(
        const std::filesystem::path& source,
        const std::filesystem::path& destination
    );

    void processBanner(
        const std::filesystem::path& source,
        const std::filesystem::path& destination
    );

private:
    std::filesystem::path m_programDirectory;

    // FFmpeg
    std::filesystem::path findFfmpegInPath() const;
    void downloadFfmpeg() const;
    bool askToDownloadFfmpeg() const;

    // Gifsicle
    std::filesystem::path findGifsicleInPath() const;
    std::filesystem::path getGifsiclePath() const;
    bool isGifsicleAvailable() const;
    std::filesystem::path ensureGifsicle();
    void downloadGifsicle() const;
    bool askToDownloadGifsicle() const;

    void processImage(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        std::uintmax_t maximumSize,
        const std::string& assetType
    );

    void compressLossless(
        const std::filesystem::path& input,
        const std::filesystem::path& output
    );

    void compressJpegLossy(
        const std::filesystem::path& input,
        const std::filesystem::path& output,
        int quality
    );

    void optimizeGif(
        const std::filesystem::path& input,
        const std::filesystem::path& output
    );

    void optimizeGifLossy(
        const std::filesystem::path& input,
        const std::filesystem::path& output,
        int lossyLevel
    );

    void reduceGifFps(
        const std::filesystem::path& input,
        const std::filesystem::path& output,
        int targetFps
    );

    static std::string getExtension(
        const std::filesystem::path& file
    );

    void reduceGifColors(
        const std::filesystem::path& input,
        const std::filesystem::path& output,
        int colors
    );

    bool processGifAtFps(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        std::uintmax_t maximumSize,
        int fps,
        double sourceFps
    );

    double getGifFps(
        const std::filesystem::path& file
    ) const;

    std::filesystem::path findFfprobeInPath() const;
    std::filesystem::path getFfprobePath() const;
};