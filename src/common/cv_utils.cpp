#include "common/cv_utils.h"

namespace psh {

bool ColorSimilar(const cv::Vec3b& c1, const cv::Vec3b& c2, int delta) {
    return (std::abs(c1[0] - c2[0]) <= delta) &&
           (std::abs(c1[1] - c2[1]) <= delta) &&
           (std::abs(c1[2] - c2[2]) <= delta);
}

cv::Mat CreateMask(const cv::Mat& img, const cv::Scalar& color, int delta) {
    double d = static_cast<double>(delta);
    cv::Scalar lower = color - cv::Scalar{d, d, d};
    cv::Scalar upper = color + cv::Scalar{d, d, d};
    cv::Mat mask;
    cv::inRange(img, lower, upper, mask);
    return mask;
}

double CalcSimilarity(const cv::Mat& img1, const cv::Mat& img2) {
    CV_Assert(img1.size() == img2.size() && img1.type() == img2.type());

    cv::Mat diff;
    cv::absdiff(img1, img2, diff);
    diff = diff.mul(diff);

    double sse = cv::sum(diff)[0];
    if (sse <= 1e-10) return 100.0;
    double mse = sse / (double)(img1.total() * img1.channels());
    double psnr = 10.0 * log10((255 * 255) / mse);
    return psnr;
}

} // namespace psh