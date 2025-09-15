#include "ocr/ocr_utils.h"

#include <tesseract/baseapi.h>
extern "C" {
#include <leptonica/allheaders.h>
}
#include <spdlog/spdlog.h>
#include <QDir>
#include <QRegularExpression>

namespace {

Pix* CvMatToPix(const cv::Mat& mat) {
    if (mat.empty()) return nullptr;

    cv::Mat tmp;
    int depth = mat.depth();

    if (mat.channels() == 1) {
        if (depth != CV_8U) {
            mat.convertTo(tmp, CV_8U);
        } else {
            tmp = mat;
        }

        Pix* p = pixCreate(tmp.cols, tmp.rows, 8);
        if (!p) return nullptr;

        for (int y = 0; y < tmp.rows; ++y) {
            const uint8_t* src = tmp.ptr<uint8_t>(y);
            l_uint32* dst = pixGetData(p) + y * pixGetWpl(p);
            for (int x = 0; x < tmp.cols; ++x) {
                SET_DATA_BYTE(dst, x, src[x]);
            }
        }
        return p;
    }

    if (mat.channels() == 3) {
        cv::cvtColor(mat, tmp, cv::COLOR_BGR2RGB);
    } else if (mat.channels() == 4) {
        cv::cvtColor(mat, tmp, cv::COLOR_BGRA2RGBA);
    } else {
        return nullptr;
    }

    Pix* p = pixCreate(tmp.cols, tmp.rows, 32);
    if (!p) return nullptr;

    for (int y = 0; y < tmp.rows; ++y) {
        const uint8_t* src = tmp.ptr<uint8_t>(y);
        l_uint32* dst = pixGetData(p) + y * pixGetWpl(p);
        for (int x = 0; x < tmp.cols; ++x) {
            const uint8_t r = src[x * tmp.channels() + 0];
            const uint8_t g = src[x * tmp.channels() + 1];
            const uint8_t b = src[x * tmp.channels() + 2];
            const uint8_t a = (tmp.channels() == 4) ? src[x * 4 + 3] : 0xFF;
            composeRGBAPixel(r, g, b, a, &dst[x]);
        }
    }
    return p;
}

std::pair<QString, int> OcrOnceWithLang(Pix* pix, const char* lang) {
    if (!pix) return {QString(), 0};
    tesseract::TessBaseAPI tess;
    QString tessdata_path = QDir::currentPath() + "/tessdata";
    if (tess.Init(tessdata_path.toLocal8Bit().constData(), lang)) {
        spdlog::error("Tesseract init failed with tessdata path: {}",
                      tessdata_path.toUtf8().constData());
        return {QString(), 0};
    }
    tess.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    tess.SetVariable("user_defined_dpi", "300");
    tess.SetVariable("preserve_interword_spaces", "1");
    tess.SetImage(pix);
    char* outText = tess.GetUTF8Text();
    QString text;
    if (outText) {
        text = QString::fromUtf8(outText).simplified();
        delete[] outText;
    }
    const int conf = tess.MeanTextConf();
    return {text, conf};
}

} // namespace

namespace psh {

cv::Mat PreprocessWhiteTextForOCR(const cv::Mat& image) {
    try {
        if (image.empty()) return image;

        cv::Mat gray;
        if (image.channels() == 3)
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        else if (image.channels() == 4)
            cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
        else
            gray = image.clone();

        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        clahe->apply(gray, gray);

        cv::Mat bin;
        cv::threshold(gray, bin, 200, 255, cv::THRESH_BINARY);

        cv::bitwise_not(bin, bin);

        return bin;
    } catch (...) {
        return image;
    }
}

QString ExtractSongNameFromImage(const cv::Mat& image) {
    const cv::Mat processed = PreprocessWhiteTextForOCR(image);
    Pix* pix = CvMatToPix(processed);
    if (!pix) return QString();

    auto [text_jpn, conf_jpn] = OcrOnceWithLang(pix, "jpn");
    pixDestroy(&pix);

    spdlog::info("OCR result (JPN): {} (conf={})",
                 text_jpn.toUtf8().constData(), conf_jpn);
    return text_jpn;
}

} // namespace psh
