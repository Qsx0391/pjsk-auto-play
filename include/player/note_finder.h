#pragma once

#ifndef NOTE_FINDER_H_
#define NOTE_FINDER_H_

#include <vector>
#include <qstring.h>

#include <opencv2/opencv.hpp>

namespace psh {

enum class NoteType { Tap = 0, Hold = 1, Slide = 2, Yellow = 3 };
QString NoteTypeToString(NoteType type);

class NoteFinder {
public:
    // clang-format off
    inline static const cv::Scalar kTapNoteColor    = {255, 243, 243};
    inline static const cv::Scalar kHoldNoteColor   = {251, 255, 243};
    inline static const cv::Scalar kSlideNoteColor  = {247, 239, 255};
    inline static const cv::Scalar kYellowNoteColor = {203, 251, 255};
    inline static const std::vector<cv::Scalar> kNoteColors = {
        kTapNoteColor, kHoldNoteColor, kSlideNoteColor, kYellowNoteColor};
    // clang-format on

    static const int kMinNoteWidth = 20;

    NoteFinder(int tolerance = 10);
    NoteFinder(const NoteFinder&) = default;
    NoteFinder(NoteFinder&&) = default;

    std::vector<cv::Point> FindNotes(const cv::Mat& img, NoteType type);
    std::vector<std::pair<NoteType, cv::Point>> FindAllNotes(const cv::Mat& img);

private:
    std::vector<cv::Scalar> lower_bound_;
    std::vector<cv::Scalar> upper_bound_;
    std::vector<std::vector<cv::Point>> note_contours_;
};

} // namespace psh

#endif // !NOTE_FINDER_H_
