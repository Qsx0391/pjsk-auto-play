#pragma once

#ifndef PSH_PLAYER_SONG_UTILS_H_
#define PSH_PLAYER_SONG_UTILS_H_

#include <vector>
#include <optional>

#include <opencv2/opencv.hpp>

#include "player/auto_play_constant.h"
#include "screen/events.h"

namespace psh {

cv::Mat GetSongNameImg(const cv::Mat& img, const Event::Area& area);

std::vector<SongStatus> FindMultiSongStatus(const cv::Mat& img);

SongDifficulty SelectMultiDifficulty(const cv::Mat& img,
                                     SongDifficulty max_diff);

std::optional<SongDifficulty> FindSoloDifficulty(const cv::Mat& img);

} // namespace psh

#endif // !PSH_PLAYER_SONG_UTILS_H_
