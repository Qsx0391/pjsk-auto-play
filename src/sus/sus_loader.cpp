#include "sus/sus_loader.h"

#include <vector>
#include <algorithm>

#include <spdlog/spdlog.h>

#include <QDir>
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QDateTime>
#include <QTimer>
#include <MikuMikuWorld/SusParser.h>

#include "common/command.h"
#include "common/finalizer.hpp"
#include "ocr/ocr_utils.h"

namespace psh {

SusLoader::SusLoader() : QObject(nullptr) {}

void SusLoader::LoadSusByName(const QString& song_name,
                              const QString& difficulty, bool force_download,
                              bool exact_match) {
    auto& inst = Instance();
    if (inst.is_loading_.exchange(true)) {
        spdlog::warn("SusLoader: load already in progress, ignoring request");
        return;
    }
    emit inst.LoadStarted();
    std::thread load_thread(
        [&inst, song_name, difficulty, force_download, exact_match]() {
            Finalizer guard([&inst]() { inst.is_loading_.store(false); });
            {
                std::lock_guard<std::mutex> lk(inst.sus_mutex_);
                inst.sus_.reset();
            }
            auto sus = inst.LoadSusImpl(song_name, difficulty, force_download,
                                        exact_match);
            {
                std::lock_guard<std::mutex> lk(inst.sus_mutex_);
                inst.sus_ = sus;
            }
            emit inst.LoadFinished();
        });
    load_thread.detach();
}

void SusLoader::LoadSusByImg(const cv::Mat& song_name_img,
                             const QString& difficulty, bool force_download,
                             bool exact_match) {
    auto& inst = Instance();
    if (inst.is_loading_.exchange(true)) {
        spdlog::warn("SusLoader: load already in progress, ignoring request");
        return;
    }
    emit inst.LoadStarted();
    std::thread load_thread([&inst, song_name_img, difficulty, force_download,
                             exact_match]() {
        Finalizer guard([&inst]() { inst.is_loading_.store(false); });
        {
            std::lock_guard<std::mutex> lk(inst.sus_mutex_);
            inst.sus_.reset();
        }
        const QString song_name = psh::ExtractSongNameFromImage(song_name_img);
        if (song_name.isEmpty()) {
            spdlog::error("SusLoader: OCR failed to extract song name");
            return;
        }
        auto sus = inst.LoadSusImpl(song_name, difficulty, force_download,
                                    exact_match);
        {
            std::lock_guard<std::mutex> lk(inst.sus_mutex_);
            inst.sus_ = sus;
        }
        emit inst.LoadFinished();
    });
    load_thread.detach();
}

SusLoader& SusLoader::Instance() {
    static SusLoader instance;
    return instance;
}

void SusLoader::RefreshSongsCache() {
    auto& inst = Instance();
    std::thread th([&inst]() {
        inst.GetSongs(true);
        spdlog::info("SusLoader: songs cache refreshed, count={}",
                     static_cast<int>(inst.songs_.size()));
    });
    th.detach();
}

std::shared_ptr<SusLoader::SusResult> SusLoader::GetSus() {
    auto& inst = Instance();
    std::lock_guard<std::mutex> lk(inst.sus_mutex_);
    return inst.sus_;
}

void SusLoader::GetSongs(bool force_refresh) {
    if (!force_refresh && !songs_.empty()) {
        return;
    }
    // 非强制刷新时，仅尝试加载本地缓存；不自动从远端获取
    if (!force_refresh) {
        LoadSongsCache();
        return;
    }

    // 强制刷新：从远端获取并写入缓存
    std::vector<SongInfo> fetched = FetchSongsFromRemote();
    songs_ = std::move(fetched);
    if (!songs_.empty()) {
        SaveSongsCache();
    }
}

bool SusLoader::LoadSongsCache() {
    QFile file(kSongsCacheFile);
    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }
    const QJsonObject obj = doc.object();
    const QJsonValue songs_val = obj.value(QStringLiteral("songs"));
    if (!songs_val.isArray()) {
        return false;
    }
    const QJsonArray arr = songs_val.toArray();
    std::vector<SongInfo> parsed;
    parsed.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        const QJsonObject s = v.toObject();
        SongInfo info;
        info.id = s.value(QStringLiteral("id")).toInt();
        info.title = s.value(QStringLiteral("title")).toString();

        const QJsonValue diffs_val = s.value(QStringLiteral("difficulties"));
        if (diffs_val.isArray()) {
            for (const QJsonValue& dv : diffs_val.toArray()) {
                const QString d = dv.toString();
                if (!d.isEmpty()) info.difficulties.push_back(d);
            }
        }
        if (info.id != 0 && !info.title.isEmpty()) {
            parsed.push_back(std::move(info));
        }
    }
    songs_.swap(parsed);
    return !songs_.empty();
}

void SusLoader::SaveSongsCache() {
    QDir().mkpath(kCacheDir);
    QFile file(kSongsCacheFile);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    QJsonArray arr;
    for (const auto& s : songs_) {
        QJsonObject o;
        o.insert(QStringLiteral("id"), s.id);
        o.insert(QStringLiteral("title"), s.title);
        if (!s.difficulties.empty()) {
            QJsonArray diffs;
            for (const auto& d : s.difficulties) diffs.push_back(d);
            o.insert(QStringLiteral("difficulties"), diffs);
        }
        arr.push_back(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("songs"), arr);
    root.insert(QStringLiteral("timestamp"),
                QDateTime::currentSecsSinceEpoch());
    root.insert(QStringLiteral("count"), static_cast<int>(songs_.size()));
    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

std::vector<SusLoader::SongInfo> SusLoader::FetchSongsFromRemote() {
    std::vector<SongInfo> result;
    const QByteArray musics_data = FetchUrlWithRetry(
        QUrl(kSekaiViewerDb + QStringLiteral("/musics.json")));
    const QByteArray diffs_data = FetchUrlWithRetry(
        QUrl(kSekaiViewerDb + QStringLiteral("/musicDifficulties.json")));

    QJsonParseError err{};
    const QJsonDocument musics_doc = QJsonDocument::fromJson(musics_data, &err);
    if (err.error != QJsonParseError::NoError || !musics_doc.isArray()) {
        return result;
    }

    QHash<int, std::vector<QString>> diffs_map;
    {
        QJsonParseError err2{};
        const QJsonDocument diffs_doc =
            QJsonDocument::fromJson(diffs_data, &err2);
        if (err2.error == QJsonParseError::NoError && diffs_doc.isArray()) {
            for (const QJsonValue& v : diffs_doc.array()) {
                const QJsonObject o = v.toObject();
                const int music_id = o.value(QStringLiteral("musicId")).toInt();
                const QString diff =
                    o.value(QStringLiteral("musicDifficulty")).toString();
                if (music_id <= 0 || diff.isEmpty()) continue;
                auto& vec = diffs_map[music_id];
                vec.push_back(diff);
            }
        }
    }

    const QJsonArray arr = musics_doc.array();
    result.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        SongInfo info;
        info.id = o.value(QStringLiteral("id")).toInt();
        info.title = o.value(QStringLiteral("title")).toString();
        if (info.id != 0 && !info.title.isEmpty()) {
            if (diffs_map.contains(info.id))
                info.difficulties = diffs_map.value(info.id);
            result.push_back(std::move(info));
        }
    }
    return result;
}

QString SusLoader::GetChartData(int song_id, const QString& difficulty) {
    spdlog::info("Downloading chart data for song id: {} difficulty: {}",
                 song_id, difficulty.toUtf8().constData());
    const QString song_id_str =
        QStringLiteral("%1_01").arg(song_id, 4, 10, QLatin1Char('0'));
    const QByteArray data = FetchUrlWithRetry(QUrl(
        kSekaiAssets + QStringLiteral("/music/music_score/") + song_id_str +
        QStringLiteral("/") + difficulty + QStringLiteral(".txt")));
    return QString::fromUtf8(data);
}

QString SusLoader::SaveChartToFile(const QString& song_title,
                                   const QString& difficulty,
                                   const QString& chart_data,
                                   const QString& output_dir) {
    QDir().mkpath(output_dir);
    const QString filename = MakeSafeFileName(
        song_title + QStringLiteral("_") + difficulty + QStringLiteral(".sus"));
    const QString filepath = QDir(output_dir).filePath(filename);
    QFile f(filepath);
    if (f.exists()) return filepath;
    if (!f.open(QIODevice::WriteOnly)) {
        spdlog::error("SusLoader: failed to open file for write: {} reason: {}",
                      filepath.toUtf8().constData(),
                      f.errorString().toUtf8().constData());
        return QString();
    }
    f.write(chart_data.toUtf8());
    f.close();
    return filepath;
}

QString SusLoader::CheckChartFileExists(const QString& song_title,
                                        const QString& difficulty,
                                        const QString& output_dir) {
    const QString filename = MakeSafeFileName(
        song_title + QStringLiteral("_") + difficulty + QStringLiteral(".sus"));
    const QString filepath = QDir(output_dir).filePath(filename);
    return QFile::exists(filepath) ? filepath : QString();
}

QString SusLoader::NormalizeText(const QString& text) const {
    if (text.isEmpty()) return QString();

    // 1. Unicode NFKC 标准化（全角半角统一）
    QString t = text.normalized(QString::NormalizationForm_KC);
    t = t.toLower();

    // 2. 保留字符范围：
    // - \w (字母数字下划线)
    // - \s (空白)
    // - 日文假名 + 汉字
    // - 小片假名 (31F0-31FF)
    // - 常见符号: ・ー〜♪!?！？()
    static const QRegularExpression re(
        QStringLiteral("[^\\w\\s"
                       "\\x{3040}-\\x{309F}" // 平假名
                       "\\x{30A0}-\\x{30FF}" // 片假名
                       "\\x{31F0}-\\x{31FF}" // 小片假名扩展
                       "\\x{4E00}-\\x{9FFF}" // CJK 基本汉字
                       "ー・〜♪!\\?！？（）()]+"));

    t.replace(re, QString());

    // 3. 压缩多余空白
    return t.simplified();
}

double SusLoader::CalculateSimilarity(const QString& query,
                                      const QString& target,
                                      bool substring_match) const {
    const QString q = query.trimmed().toLower();
    const QString t = target.trimmed().toLower();
    if (q.isEmpty() || t.isEmpty()) return 0.0;
    if (q == t) return 1.0;
    const int n = q.size();
    const int m = t.size();
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1));
    for (int i = 0; i <= n; ++i) dp[i][0] = i;
    for (int j = 0; j <= m; ++j) dp[0][j] = j;
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            int cost = (q[i - 1] == t[j - 1]) ? 0 : 1;
            dp[i][j] = std::min(
                {dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    int distance = dp[n][m];
    return substring_match
               ? 1.0 - static_cast<double>(distance - std::abs(m - n)) /
                           std::min(n, m)
               : 1.0 - static_cast<double>(distance) / std::max(n, m);
}

std::vector<std::pair<SusLoader::SongInfo, double>> SusLoader::SearchSongByName(
    const QString& song_name, bool exact_match) {
    GetSongs(false);
    std::vector<SongInfo> songs = songs_;
    std::vector<std::pair<SongInfo, double>> scored;

    if (exact_match) {
        const QString nq = NormalizeText(song_name);
        for (const auto& s : songs) {
            if (NormalizeText(s.title) == nq) {
                scored.emplace_back(s, 1.0);
            }
        }
    } else {
        auto tryMatch = [&]() {
            auto& list = songs;

            using Pair = std::pair<double, SongInfo>;
            auto cmp = [](const Pair& a, const Pair& b) {
                return a.first > b.first;
            };
            std::priority_queue<Pair, std::vector<Pair>, decltype(cmp)> pq(cmp);

            for (const auto& s : list) {
                const double sim =
                    song_name.size() >= 10 && s.title.size() > song_name.size()
                        ? CalculateSimilarity(song_name, s.title, true)
                        : CalculateSimilarity(song_name, s.title, false);
                pq.emplace(sim, s);
                if (pq.size() > 5) pq.pop();
            }

            std::vector<std::pair<SongInfo, double>> topk;
            topk.reserve(pq.size());
            while (!pq.empty()) {
                topk.emplace_back(pq.top().second, pq.top().first);
                pq.pop();
            }
            std::reverse(topk.begin(), topk.end());
            return topk;
        };

        scored = tryMatch();
    }

    std::vector<std::pair<SusLoader::SongInfo, double>> out;
    out.reserve(scored.size());
    for (const auto& p : scored) out.push_back(p);
    return out;
}

std::shared_ptr<SusLoader::SusResult> SusLoader::LoadSusImpl(
    const QString& song_name, const QString& difficulty, bool force_download,
    bool exact_match) {
    // 搜索歌曲
    auto matches = SearchSongByName(song_name, exact_match);
    if (!exact_match) {
        emit MatchesUpdated(matches);
    }
    if (matches.empty()) {
        spdlog::error("SusLoader: cannot find song for query: {}",
                      song_name.toUtf8().constData());
        return nullptr;
    }
    const auto& [selected, sim] = matches.front();
    spdlog::info("SusLoader: selected song: {} (id={}, sim={:.2f})",
                 selected.title.toUtf8().constData(), selected.id, sim);

    // 校验难度
    if (difficulty.isEmpty()) {
        return nullptr;
    }
    if (std::find(selected.difficulties.begin(), selected.difficulties.end(),
                  difficulty) == selected.difficulties.end()) {
        QStringList list;
        for (const auto& d : selected.difficulties) list << d;
        spdlog::error("SusLoader: difficulty not available: {}, available: {}",
                      difficulty.toUtf8().constData(),
                      list.join(", ").toUtf8().constData());
        return nullptr;
    }

    QString song_info_str = selected.title + " [" + difficulty + "]";

    // 若不强制下载，则先看文件是否已存在
    if (!force_download) {
        const QString existing =
            CheckChartFileExists(selected.title, difficulty);
        if (!existing.isEmpty()) {
            MikuMikuWorld::SusParser parser;
            auto sus = parser.parse(existing.toStdString());
            spdlog::info("SusLoader: loaded cached sus: {}, diff: {}",
                         selected.title.toUtf8().constData(),
                         difficulty.toUtf8().constData());
            auto result = std::make_shared<SusResult>();
            result->sus = std::move(sus);
            result->title = selected.title;
            result->difficulty = difficulty;
            emit SusUpdated(result);
            return result;
        }
    }

    // 下载谱面
    const QString chart = GetChartData(selected.id, difficulty);
    if (chart.isEmpty()) {
        spdlog::error(
            "SusLoader: failed to download chart data for song: {} diff: {}",
            selected.title.toUtf8().constData(),
            difficulty.toUtf8().constData());
        return nullptr;
    }

    const QString path = SaveChartToFile(selected.title, difficulty, chart);
    if (path.isEmpty()) {
        spdlog::error("SusLoader: failed to save chart file");
        return nullptr;
    }

    // 解析
    MikuMikuWorld::SusParser parser;
    auto sus = parser.parse(path.toStdString());
    spdlog::info("SusLoader: successfully loaded sus file: {}",
                 path.toUtf8().constData());
    auto result = std::make_shared<SusResult>();
    result->sus = std::move(sus);
    result->title = selected.title;
    result->difficulty = difficulty;
    emit SusUpdated(result);
    return result;
}

QString SusLoader::MakeSafeFileName(const QString& base) const {
    static const QRegularExpression invalid(
        QStringLiteral("[\\\\/:*?\"<>|\\x00-\\x1F]"));
    QString sanitized = base;
    sanitized.replace(invalid, QStringLiteral("_"));
    while (!sanitized.isEmpty() &&
           (sanitized.endsWith(' ') || sanitized.endsWith('.'))) {
        sanitized.chop(1);
    }
    if (sanitized.isEmpty()) sanitized = QStringLiteral("file");
    return sanitized;
}

QByteArray SusLoader::FetchUrlWithRetry(const QUrl& initial_url) const {
    spdlog::info("SusLoader: fetching url: {}",
                 initial_url.toString().toUtf8().constData());
    QNetworkAccessManager mgr;
    QEventLoop loop;
    QObject::connect(&mgr, &QNetworkAccessManager::finished, &loop,
                     &QEventLoop::quit);
    QUrl url = initial_url;
    QByteArray data;
    const int max_attempts = 3;
    QString last_error_string;
    int last_http_status = 0;
    int last_qnet_error = 0;
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("pjsk-auto-play/1.0"));
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply* reply = mgr.get(req);
        QTimer timer;
        timer.setSingleShot(true);
        bool timed_out = false;
        QObject::connect(&timer, &QTimer::timeout, [&]() {
            if (reply && reply->isRunning()) {
                timed_out = true;
                reply->abort();
            }
        });
        timer.start(10000);
        loop.exec();
        timer.stop();
        const QVariant http_status_var =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        last_http_status =
            http_status_var.isValid() ? http_status_var.toInt() : 0;
        last_qnet_error = static_cast<int>(reply->error());
        last_error_string = reply->errorString();
        if (reply->error() == QNetworkReply::NoError) {
            data = reply->readAll();
            reply->deleteLater();
            break;
        }
        if (timed_out) {
            spdlog::warn("SusLoader: request timeout (attempt {}/{}): "
                         "http_status={}",
                         attempt, max_attempts, last_http_status);
        } else {
            spdlog::warn("SusLoader: request failed (attempt {}/{}): "
                         "qnet_error={} reason={} http_status={}",
                         attempt, max_attempts, last_qnet_error,
                         last_error_string.toUtf8().constData(),
                         last_http_status);
        }
        const QVariant redir =
            reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redir.isValid()) {
            const QUrl new_url = url.resolved(redir.toUrl());
            spdlog::info("SusLoader: redirecting to {}",
                         new_url.toString().toUtf8().constData());
            url = new_url;
        }
        reply->deleteLater();
        if (attempt == max_attempts) {
            spdlog::error("SusLoader: all attempts failed: url={}",
                          initial_url.toString().toUtf8().constData());
            return QByteArray();
        }
    }
    return data;
}

} // namespace psh