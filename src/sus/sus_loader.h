#pragma once

#ifndef PSH_SUS_SUS_LOADER
#define PSH_SUS_SUS_LOADER

#include <memory>
#include <vector>
#include <future>
#include <mutex>
#include <atomic>
#include <functional>

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QObject>
#include <opencv2/opencv.hpp>
#include <MikuMikuWorld/SUS.h>

namespace psh {

class SusLoader : public QObject {
public:
    Q_OBJECT

public:
    struct SongInfo {
        int id = 0;
        QString title;
        std::vector<QString> difficulties;
    };

    struct SusResult {
        MikuMikuWorld::SUS sus;
        QString title;
        QString difficulty;
    };

    ~SusLoader() = default;

    static SusLoader& Instance();

    static void LoadSusByName(const QString& song_name,
                              const QString& difficulty,
                              bool force_download = false,
                              bool exact_match = false);
    static void LoadSusByImg(const cv::Mat& song_name_img,
                             const QString& difficulty,
                             bool force_download = false,
                             bool exact_match = false);
    static std::shared_ptr<SusResult> GetSus();

    static void RefreshSongsCache();

signals:
    void LoadStarted();
    void LoadFinished();
    void MatchesUpdated(
        const std::vector<std::pair<SongInfo, double>>& matches);
    void SusUpdated(std::shared_ptr<SusResult> result);

private:
    SusLoader();
    SusLoader(const SusLoader&) = delete;
    SusLoader& operator=(const SusLoader&) = delete;
    SusLoader(SusLoader&&) = delete;
    SusLoader& operator=(SusLoader&&) = delete;

    // clang-format off
    inline static const QString kSekaiViewerDb  = QStringLiteral("https://sekai-world.github.io/sekai-master-db-diff");
    inline static const QString kSekaiAssets    = QStringLiteral("https://storage.sekai.best/sekai-jp-assets");
    inline static const QString kCacheDir       = QStringLiteral("cache");
    inline static const QString kSongsCacheFile = QStringLiteral("cache/songs_cache.json");
    // clang-format on

    void GetSongs(bool force_refresh = false);
    bool LoadSongsCache();
    void SaveSongsCache();
    std::vector<SongInfo> FetchSongsFromRemote();
    QString GetChartData(int song_id, const QString& difficulty);
    QString SaveChartToFile(
        const QString& song_title, const QString& difficulty,
        const QString& chart_data,
        const QString& output_dir = QStringLiteral("charts"));
    QString CheckChartFileExists(
        const QString& song_title, const QString& difficulty,
        const QString& output_dir = QStringLiteral("charts"));

    QString NormalizeText(const QString& text) const;
    double CalculateSimilarity(const QString& query, const QString& target,
                               bool substring_match) const;
    std::vector<std::pair<SongInfo, double>> SearchSongByName(
        const QString& song_name, bool exact_match = false);
    std::shared_ptr<SusLoader::SusResult> LoadSusImpl(const QString& song_name,
                                                      const QString& difficulty,
                                                      bool force_download,
                                                      bool exact_match);

    QString MakeSafeFileName(const QString& base) const;
    QByteArray FetchUrlWithRetry(const QUrl& url) const;

    std::vector<SongInfo> songs_;
    std::shared_ptr<SusResult> sus_;
    std::mutex sus_mutex_;
    std::atomic<bool> is_loading_ = false;
};

} // namespace psh

#endif // !PSH_SUS_SUS_LOADER
