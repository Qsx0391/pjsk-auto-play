#include "player/song_utils.h"

#include <spdlog/spdlog.h>

#include "screen/events.h"
#include "common/cv_utils.h"

namespace psh {

cv::Mat GetSongNameImg(const cv::Mat& img, const Event::Area& area) {
    std::vector<cv::Point2f> dstPts = {
        cv::Point2f(0, 0), cv::Point2f(area.width - 1, 0),
        cv::Point2f(area.width - 1, area.height - 1),
        cv::Point2f(0, area.height - 1)};

    cv::Mat Minv;
    cv::invert(area.M, Minv);
    std::vector<cv::Point2f> srcPts;
    cv::perspectiveTransform(dstPts, srcPts, Minv);

    cv::Rect roi =
        cv::boundingRect(srcPts) & cv::Rect(0, 0, img.cols, img.rows);
    cv::Mat crop = img(roi).clone();

    cv::resize(crop, crop, cv::Size(), kSongNameScale, kSongNameScale,
               cv::INTER_CUBIC);

    std::vector<cv::Point2f> srcPts_cropped;
    for (auto& pt : srcPts) {
        srcPts_cropped.push_back((pt - cv::Point2f(roi.x, roi.y)) *
                                 kSongNameScale);
    }

    std::vector<cv::Point2f> dstPts_scaled;
    for (auto& pt : dstPts) {
        dstPts_scaled.push_back(pt * kSongNameScale);
    }

    cv::Mat M_cropped =
        cv::getPerspectiveTransform(srcPts_cropped, dstPts_scaled);

    cv::Mat warped;
    cv::warpPerspective(
        crop, warped, M_cropped,
        cv::Size(area.width * kSongNameScale, area.height * kSongNameScale),
        cv::INTER_LANCZOS4, cv::BORDER_REPLICATE);

    return warped;
}

std::vector<SongStatus> FindMultiSongStatus(const cv::Mat& img) {
    std::vector<SongStatus> res;
    const auto& event = Events::GetEvent("multi_play_start");

    for (int i = 0; i < static_cast<int>(SongDifficulty::Count); ++i) {
        auto diff = static_cast<SongDifficulty>(i);
        QString point_name = QString("%1_status").arg(kDifficultyStrs[i]);
        cv::Point pos = event.GetPoint(point_name);
        cv::Vec3b color = img.at<cv::Vec3b>(pos);

        auto best = std::min_element(
            kSongStatusColors.begin(), kSongStatusColors.end(),
            [&](const auto& a, const auto& b) {
                return cv::norm(color - a) < cv::norm(color - b);
            });

        res.push_back(
            static_cast<SongStatus>(best - kSongStatusColors.begin()));
    }
    return res;
}

SongDifficulty SelectMultiDifficulty(const cv::Mat& img,
                                     SongDifficulty max_diff) {
    std::vector<SongStatus> status = FindMultiSongStatus(img);
    QString status_str = "";
    for (const auto& s : status) {
        status_str += QString::number(static_cast<int>(s)) + " ";
    }
    spdlog::info("Find song status: {}", status_str.toUtf8().constData());
    int target = static_cast<int>(max_diff);
    for (int i = target - 1; i >= 0; --i) {
        if (status[i] < status[target]) {
            target = i;
        }
    }
    return static_cast<SongDifficulty>(target);
}

std::optional<SongDifficulty> FindSoloDifficulty(const cv::Mat& img) {
    const auto& event = Events::GetEvent("solo_play_start");
    cv::Point pos = event.GetPoint("difficulty");

    cv::Vec3b color = img.at<cv::Vec3b>(pos);
    for (int j = 0; j < kDifficultyColors.size(); ++j) {
        if (ColorSimilar(color, kDifficultyColors[j])) {
            return static_cast<SongDifficulty>(j);
        }
    }
    return std::nullopt;
}

} // namespace psh