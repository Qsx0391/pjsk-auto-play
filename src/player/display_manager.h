#pragma once

#ifndef PSH_PLAYER_DISPLAY_MANAGER_H_
#define PSH_PLAYER_DISPLAY_MANAGER_H_

#include <atomic>
#include <functional>

#include <QObject>

#include "player/note_finder.h"
#include "touch/i_touch.h"

namespace psh {

class DisplayManager : public QObject {
public:
    Q_OBJECT
public:
    ~DisplayManager() = default;

    static void StartDisplay();
    static void StopDisplay();

    static bool UpdateDisplay(
        const cv::Mat& frame, TouchController& touch,
        const std::vector<std::pair<NoteColor, std::vector<Note>>>& notes);
        
    static DisplayManager& Instance();

signals:
    void displayOff();

private:
    DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    DisplayManager(DisplayManager&&) = delete;
    DisplayManager& operator=(DisplayManager&&) = delete;

    void CloseWindow();

    std::atomic_bool display_enable_{false};
    bool window_created_ = false;
};

} // namespace psh

#endif // !PSH_PLAYER_DISPLAY_MANAGER_H_
