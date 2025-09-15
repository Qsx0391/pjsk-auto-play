#include "screen/events.h"

#include <spdlog/spdlog.h>

#include "common/time_utils.h"
#include "common/cv_utils.h"

namespace psh {

const Event& Events::GetEvent(const QString& name) {
    auto& inst = instance();
    if (!inst.events_.contains(name)) {
        throw std::runtime_error("Invalid event name");
    }
    return inst.events_[name];
}

const Event* Events::MatchEventByName(const std::vector<QString>& names,
                                      cv::Mat img, double min_psnr) {
    for (const auto& name : names) {
        const auto& event = GetEvent(name);
        if (event.Check(img, min_psnr)) {
            return &event;
        }
    }
    return nullptr;
}

Events& Events::instance() {
    static Events inst;
    return inst;
}

void Events::loadEvents() {
    QDir eventsDir(":/events");
    QStringList eventNames =
        eventsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& eventName : eventNames) {
        QDir evDir(eventsDir.filePath(eventName));
        Event ev;
        ev.name_ = eventName;

        QStringList pngs = evDir.entryList({"*.png"}, QDir::Files);
        for (const QString& png : pngs) {
            QString base = png;
            base.chop(4);
            QStringList parts = base.split("_");
            if (parts.size() == 2) {
                bool okx, oky;
                int x = parts[0].toInt(&okx);
                int y = parts[1].toInt(&oky);
                if (okx && oky) {
                    QFile f(evDir.filePath(png));
                    if (f.open(QIODevice::ReadOnly)) {
                        QByteArray data = f.readAll();
                        std::vector<uchar> buf(data.begin(), data.end());
                        ev.check_img_.img = cv::imdecode(buf, cv::IMREAD_COLOR);
                        ev.check_img_.pos = cv::Point(x, y);
                    }
                    break;
                }
            }
        }

        QFile jsonFile(evDir.filePath("event.json"));
        if (jsonFile.exists() && jsonFile.open(QIODevice::ReadOnly)) {
            QByteArray data = jsonFile.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject root = doc.object();

                // buttons
                if (root.contains("buttons")) {
                    QJsonObject obj = root["buttons"].toObject();
                    for (auto it = obj.begin(); it != obj.end(); ++it) {
                        QJsonArray arr = it.value().toArray();
                        if (arr.size() == 2)
                            ev.btns_[it.key()] =
                                cv::Point(arr[0].toInt(), arr[1].toInt());
                    }
                }

                // points
                if (root.contains("points")) {
                    QJsonObject obj = root["points"].toObject();
                    for (auto it = obj.begin(); it != obj.end(); ++it) {
                        QJsonArray arr = it.value().toArray();
                        if (arr.size() == 2)
                            ev.points_[it.key()] =
                                cv::Point(arr[0].toInt(), arr[1].toInt());
                    }
                }

                // rects
                if (root.contains("rects")) {
                    QJsonObject obj = root["rects"].toObject();
                    for (auto it = obj.begin(); it != obj.end(); ++it) {
                        QJsonObject robj = it.value().toObject();
                        if (robj.contains("tl") && robj.contains("width") &&
                            robj.contains("height")) {
                            QJsonArray tl = robj["tl"].toArray();
                            if (tl.size() == 2) {
                                int x = tl[0].toInt();
                                int y = tl[1].toInt();
                                int w = robj["width"].toInt();
                                int h = robj["height"].toInt();
                                ev.rects_[it.key()] = cv::Rect(x, y, w, h);
                            }
                        }
                    }
                }

                // areas
                if (root.contains("areas")) {
                    QJsonObject obj = root["areas"].toObject();
                    for (auto it = obj.begin(); it != obj.end(); ++it) {
                        QJsonArray arr = it.value().toArray();
                        if (arr.size() == 4) {
                            std::vector<cv::Point2f> src_pts;
                            for (int i = 0; i < 4; i++) {
                                QJsonArray pt = arr[i].toArray();
                                if (pt.size() == 2) {
                                    src_pts.emplace_back(pt[0].toInt(),
                                                         pt[1].toInt());
                                }
                            }
                            if (src_pts.size() == 4) {
                                float width1 =
                                    cv::norm(src_pts[0] - src_pts[1]);
                                float width2 =
                                    cv::norm(src_pts[2] - src_pts[3]);
                                float height1 =
                                    cv::norm(src_pts[0] - src_pts[3]);
                                float height2 =
                                    cv::norm(src_pts[1] - src_pts[2]);
                                int width =
                                    static_cast<int>(std::max(width1, width2));
                                int height = static_cast<int>(
                                    std::max(height1, height2));

                                std::vector<cv::Point2f> dst_pts = {
                                    cv::Point2f(0, 0),
                                    cv::Point2f(width - 1, 0),
                                    cv::Point2f(width - 1, height - 1),
                                    cv::Point2f(0, height - 1)};

                                Event::Area area;
                                area.M = cv::getPerspectiveTransform(src_pts,
                                                                     dst_pts);
                                area.width = width;
                                area.height = height;

                                ev.areas_[it.key()] = area;
                            }
                        }
                    }
                }
            }
        }

        events_[eventName] = ev;
    }
}

bool Event::Check(const cv::Mat& img, double min_psnr) const {
    if (check_img_.img.empty()) {
        spdlog::error("Check image is empty");
        return false;
    }

    cv::Rect check_rect(check_img_.pos, check_img_.img.size());

    if (check_rect.x < 0 || check_rect.y < 0 ||
        check_rect.x + check_rect.width > img.cols ||
        check_rect.y + check_rect.height > img.rows) {
        return false;
    }

    cv::Mat roi = img(check_rect);
    return CalcSimilarity(roi, check_img_.img) >= min_psnr;
}

cv::Point Event::GetPoint(const QString& name) const {
    if (!points_.contains(name)) {
        spdlog::error("Point '{}' not found", name.toStdString());
        return cv::Point(-1, -1);
    }
    return points_[name];
}

cv::Point Event::GetButton(const QString& name) const {
    if (!btns_.contains(name)) {
        spdlog::error("Button '{}' not found", name.toStdString());
        return cv::Point(-1, -1);
    }
    return btns_[name];
}

cv::Mat Event::GetRect(const cv::Mat& img, const QString& name) const {
    if (!rects_.contains(name)) {
        spdlog::error("Rect '{}' not found", name.toStdString());
        return cv::Mat();
    }
    cv::Rect r = rects_.value(name);
    if (r.x < 0 || r.y < 0 || r.x + r.width > img.cols ||
        r.y + r.height > img.rows) {
        return cv::Mat();
    }
    return img(r).clone();
}

Event::Area Event::GetArea(const QString& name) const {
    if (!areas_.contains(name)) {
        spdlog::error("Area '{}' not found", name.toStdString());
        return {};
    }
    return areas_[name];
}

} // namespace psh