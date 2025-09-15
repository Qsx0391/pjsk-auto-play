#include "player/note_finder.h"

#include "common/cv_utils.h"

namespace {

cv::Point CenterOf(const cv::Rect &rect) { return (rect.tl() + rect.br()) / 2; }

} // namespace

namespace psh {

NoteFinder::NoteFinder(NoteTimeEstimator &estimator,
                       const TrackConfig &track_config)
    : estimator_(estimator), tc_(track_config) {
    HrLine track_upper = TrackLineOf(0);
    HrLine track_lower = TrackLineOf(tc_.height);
    HrLine check_upper = TrackLineOf(tc_.check_upper_y);
    HrLine check_lower = TrackLineOf(tc_.check_lower_y);
    hit_line_ = TrackLineOf(tc_.hit_line_y);

    auto build_mask = [](const HrLine& upper, const HrLine& lower, cv::Rect& area, cv::Mat& mask) {
		area = cv::Rect(lower.pos.x, upper.pos.y, lower.length,
						lower.pos.y - upper.pos.y);
		mask = cv::Mat::zeros(area.height, area.width, CV_8UC1);
		std::vector<std::vector<cv::Point>> mask_pts = {
			{upper.Left() - area.tl(), upper.Right() - area.tl(),
			 lower.Right() - area.tl(), lower.Left() - area.tl()}};
		cv::fillPoly(mask, mask_pts, cv::Scalar(255));
	};

    build_mask(track_upper, track_lower, track_area_, track_mask_);
    build_mask(check_upper, check_lower, check_area_, check_mask_);
}

std::vector<std::pair<NoteColor, std::vector<Note>>> NoteFinder::FindAllNotes(
    const Frame &frame) {
    cv::Mat check_img;
    frame.img(check_area_).copyTo(check_img, check_mask_);
    std::vector<std::pair<NoteColor, std::vector<Note>>> ret;

    for (int i = 0; i < kNoteColors.size(); ++i) {
        NoteColor color_enum = static_cast<NoteColor>(i);
        const cv::Scalar &color = kNoteColors[i];

        std::vector<Note> notes;
        cv::Mat mask = CreateMask(check_img, color, kTapColorDelta);
        cv::findContours(mask, note_contours_, cv::RETR_EXTERNAL,
                         cv::CHAIN_APPROX_SIMPLE);
        for (const auto &contour : note_contours_) {
            cv::Rect box = cv::boundingRect(contour) + check_area_.tl();
            cv::Point pos = CenterOf(box);
            if (box.width >= kMinNoteWidth) {
                // clang-format off
                Note note;
                note.color       = color_enum;
                note.box         = box;
                note.hit_pos     = hit_line_.PosOf(TrackLineOf(pos.y), pos);
                note.hit_time_ms = estimator_.EstimateHitTime(pos.y) + frame.capture_time_ms;
                note.hold        = FindHoldType(frame, color_enum, box);
                note.is_slide    = color_enum == NoteColor::Red || color_enum == NoteColor::Yellow;
                // clang-format on
                notes.push_back(note);
            }
        }
        ret.emplace_back(color_enum, std::move(notes));
    }

    return ret;
}

cv::Mat NoteFinder::GetTrackImg(const cv::Mat &img) const { 
	cv::Mat track_img;
	img(track_area_).copyTo(track_img, track_mask_);
	return track_img;
}

HoldType NoteFinder::FindHoldType(const Frame &frame, NoteColor color,
                                  cv::Rect rect) {
    if (color == NoteColor::Blue) {
        return HoldType::None;
    }

    HoldColor hold_color =
        color == NoteColor::Yellow ? HoldColor::Yellow : HoldColor::Green;
    const cv::Vec3b &target = kHoldColors[static_cast<int>(hold_color)];

    cv::Vec3b lower = frame.img.at<cv::Vec3b>(
        CenterOf(rect) + cv::Point{0, kHoldCheckDY + rect.height / 2});
    if (ColorSimilar(lower, target, kHoldColorDelta)) {
        return HoldType::HoldEnd;
    }

    cv::Vec3b upper = frame.img.at<cv::Vec3b>(
        CenterOf(rect) - cv::Point{0, kHoldCheckDY + rect.height / 2});
    if (ColorSimilar(upper, target, kHoldColorDelta)) {
        return HoldType::HoldBegin;
    }

    return HoldType::None;
}

HrLine NoteFinder::TrackLineOf(int line_y) const {
    int a =
        (tc_.lower_len - tc_.upper_len) * (tc_.height - line_y) / tc_.height;
    return HrLine{cv::Point{tc_.dx + a / 2, line_y}, tc_.lower_len - a};
}

} // namespace psh