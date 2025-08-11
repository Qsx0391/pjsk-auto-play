#pragma once

#ifndef IMAGES_H_
#define IMAGES_H_

#include <mutex>
#include <qstring.h>

#include <opencv2/opencv.hpp>

namespace psh {

class Image {
public:
    explicit Image(const QString& path) : path_(path) {}
    const cv::Mat& data();
    bool Check(const cv::Mat& frame, double threshold = 0.9);

private:
    const QString path_;
    cv::Mat data_;
    std::mutex mutex_;
};

class Images {
public:
    static Image& SongStart1() {
        static Image img(":/PJSKAutoPlay/img/song_start/song_start_1.png");
        return img;
    }

    static Image& SongStart2() {
        static Image img(":/PJSKAutoPlay/img/song_start/song_start_2.png");
        return img;
    }

    static Image& SongPlaying1() {
        static Image img(":/PJSKAutoPlay/img/song_playing/song_playing_1.png");
        return img;
    }

    static Image& MultiPlayBegin1() {
        static Image img(
            ":/PJSKAutoPlay/img/multi_play_begin/multi_play_begin_1.png");
        return img;
    }

    static Image& MultiPlaySongChoose1() {
        static Image img(":/PJSKAutoPlay/img/multi_play_song_choose/"
                         "multi_play_song_choose_1.png");
        return img;
    }

    static Image& MultiPlaySongChoose2() {
        static Image img(":/PJSKAutoPlay/img/multi_play_song_choose/"
                         "multi_play_song_choose_2.png");
        return img;
    }

private:
    Images() = delete;
};

} // namespace psh

#endif // !IMAGES_H_
