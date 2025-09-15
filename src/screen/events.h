#pragma once

#ifndef PSH_SCREEN_ENENTS_H_
#define PSH_SCREEN_ENENTS_H_

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QHash>
#include <QString>
#include <QResource>

#include <opencv2/opencv.hpp>

namespace psh {

class Event {
public:
    struct Area {
        cv::Mat M;
        int width;
        int height;
    };

    Event() = default;

    const QString& GetName() const { return name_; }
    bool Check(const cv::Mat& img, double min_psnr = 35.0) const;
    cv::Point GetPoint(const QString& name) const;
    cv::Point GetButton(const QString& name) const;
    cv::Mat GetRect(const cv::Mat& img, const QString& name) const;
    Area GetArea(const QString& name) const;

private:
    friend class Events;

    struct CheckImg {
        cv::Mat img;
        cv::Point pos;
    };

    QString name_;
    CheckImg check_img_;
    QHash<QString, cv::Point> points_;
    QHash<QString, cv::Point> btns_;
    QHash<QString, cv::Rect> rects_;
    QHash<QString, Area> areas_;
};

class Events {
public:
    static const Event& GetEvent(const QString& name);
    static const Event* MatchEventByName(const std::vector<QString>& names,
                                         cv::Mat img, double min_psnr = 35.0);

private:
    Events() { loadEvents(); }

    static Events& instance();
    void loadEvents();

    QHash<QString, Event> events_;
};

} // namespace psh

#endif // !PSH_SCREEN_ENENTS_H_
