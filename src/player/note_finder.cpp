#include "note_finder.h"

namespace psh {

QString NoteTypeToString(NoteType type) {
    switch (type) {
        case NoteType::Tap: return "Tap";
        case NoteType::Hold: return "Hold";
        case NoteType::Slide: return "Slide";
        case NoteType::Yellow: return "Yellow";
        default: return "Unknown";
    }
}

NoteFinder::NoteFinder(int tolerance)
    : lower_bound_(kNoteColors), upper_bound_(kNoteColors) {
    float t = static_cast<float>(tolerance);
    for (int i = 0; i < kNoteColors.size(); ++i) {
        lower_bound_[i] -= cv::Scalar{t, t, t};
        upper_bound_[i] += cv::Scalar{t, t, t};
    }
}

std::vector<cv::Point> NoteFinder::FindNotes(const cv::Mat &img,
                                             NoteType type) {
    int index = static_cast<int>(type);
    cv::Mat mask;
    cv::inRange(img, lower_bound_[index], upper_bound_[index], mask);
    cv::findContours(mask, note_contours_, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);
    std::vector<cv::Point> ret;
    for (const auto &contour : note_contours_) {
        cv::Rect note_box = cv::boundingRect(contour);
        if (note_box.width < kMinNoteWidth) {
            continue;
        }
        int x = note_box.x + note_box.width / 2;
        int y = note_box.y + note_box.height / 2;
        ret.emplace_back(x, y);
    }
    /*
    if (type == NoteType::Tap) {
        for (const auto& pos : ret) {
            cv::circle(mask, pos, 2, cv::Scalar(0, 0, 255), -1);
        }
        cv::imshow("test", mask);
        cv::waitKey(1);
    }
    */
    return ret;
}

std::vector<std::pair<NoteType, cv::Point>> NoteFinder::FindAllNotes(
    const cv::Mat &img) {
    std::vector<std::pair<NoteType, cv::Point>> ret;
    for (int i = 0; i < kNoteColors.size(); ++i) {
		std::vector<cv::Point> notes = FindNotes(img, static_cast<NoteType>(i));
		for (const auto &note : notes) {
			ret.emplace_back(static_cast<NoteType>(i), note);
		}
	}
    return ret;
}

} // namespace psh