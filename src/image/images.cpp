#include "images.h"

#include <QImage>

namespace psh {

const cv::Mat &Image::data() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.empty()) {
        QImage image(path_);
        if (image.isNull()) {
            throw std::runtime_error("Failed to load image: " +
                                     path_.toStdString());
        }

        QImage formatted = image.convertToFormat(QImage::Format_RGB888);
        data_ = cv::Mat(formatted.height(), formatted.width(), CV_8UC3,
                        const_cast<uchar *>(formatted.bits()),
                        formatted.bytesPerLine())
                    .clone();
        
        cv::cvtColor(data_, data_, cv::COLOR_RGB2BGR);
    }
    return data_;
}

bool Image::Check(const cv::Mat &frame, double threshold) {
    cv::Mat result;
    cv::matchTemplate(frame, data(), result, cv::TM_CCOEFF_NORMED);
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
    return maxVal >= threshold;
}

} // namespace psh