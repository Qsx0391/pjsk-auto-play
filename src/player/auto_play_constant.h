#pragma once

#ifndef PSH_PLAYER_AUTO_PLAY_CONSTANT_H_
#define PSH_PLAYER_AUTO_PLAY_CONSTANT_H_

#include <cstdint>
#include <vector>

#include <qstring.h>
#include <opencv2/opencv.hpp>

namespace psh {

enum class NoteColor { Blue, Green, Red, Yellow, Count };

enum class HoldColor { Green, Yellow, Count };

enum class HoldType { None, HoldBegin, HoldEnd, Count };

enum class SongStatus { kUnplayed, kClear, kFullCombo, kAllPerfect, Count };

enum class SongDifficulty { kEasy, kNormal, kHard, kExpert, kMaster, kAppend, Count };

// clang-format off
constexpr int kTapColorDelta  = 3;
constexpr int kHoldColorDelta = 20;

constexpr int kSlideMoveDY		   = -200;
constexpr int kSlideDurationMs	   = 30;
constexpr int kSlideStepDelayMs	   = 1;
constexpr int kSlideDelayMs		   = -20;
constexpr int kHoldEndSlideDelayMs = -40;
constexpr int kTapDurationMs	   = 20;
constexpr int kHoldDelayMs		   = -60;

constexpr int kMinSampleCount = 5;
constexpr int kMinNoteDT      = 20;
constexpr int kMinNoteDX      = 50;
constexpr int kMinNoteDW      = 50;

constexpr int64_t kMinLoopWaitTimeMs = 1;
constexpr int64_t kMainLoopDelayMs   = 200;
constexpr int64_t kDisplayDelayMs    = 1000 / 60;

constexpr double kSongNameScale = 2.0;

inline const cv::Vec3b kAliveColor = cv::Vec3b{153, 255, 136};
inline const cv::Vec3b kDeadColor  = cv::Vec3b{ 76,  50,  50};

inline const std::array<cv::Vec3b, static_cast<int>(NoteColor::Count)> kNoteColors = {
	cv::Vec3b{255, 243, 243}, // Blue
	cv::Vec3b{251, 255, 243}, // Green
	cv::Vec3b{247, 239, 255}, // Red
	cv::Vec3b{203, 251, 255}  // Yellow
};

inline const std::array<cv::Vec3b, static_cast<int>(HoldColor::Count)> kHoldColors = {
	cv::Vec3b{180, 189, 112}, // Green
	cv::Vec3b{176, 190, 178}  // Yellow
};

inline const std::array<cv::Vec3b, static_cast<int>(SongDifficulty::Count)> kDifficultyColors = {
	cv::Vec3b{119, 221,  17}, // Easy
	cv::Vec3b{255, 204,  51}, // Normal
	cv::Vec3b{  0, 204, 255}, // Hard
	cv::Vec3b{119,  68, 255}, // Expert
	cv::Vec3b{255,  51, 204}, // Master
	cv::Vec3b{247, 103, 235}  // Append
};

inline const std::array<QString, static_cast<int>(SongDifficulty::Count)> kDifficultyStrs = {
	"easy", "normal", "hard", "expert", "master", "append"
};

inline const std::array<cv::Vec3b, static_cast<int>(SongStatus::Count)> kSongStatusColors = {
	cv::Vec3b{102,  68,  68}, // Unplayed
	cv::Vec3b{113, 226, 254}, // Clear
	cv::Vec3b{251, 170, 255}, // Full Combo
	cv::Vec3b{206, 204,  19}  // All Perfect
};

inline const std::string kWindowName = "Screen Show";
// clang-format on

} // namespace psh

#endif // !PSH_PLAYER_AUTO_PLAY_CONSTANT_H_