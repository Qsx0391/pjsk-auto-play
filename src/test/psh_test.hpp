#pragma once

#ifndef PSH_TEST_PSH_TEST_HPP_
#define PSH_TEST_PSH_TEST_HPP_

#include <spdlog/spdlog.h>

#include "touch/mini_touch_client.h"
#include "screen/i_screen.h"
#include "player/note_finder.h"
#include "player/auto_player.h"
#include "mumu/mumu_client.h"
#include "sus/score_touch.h"
#include "common/time_utils.h"

namespace psh::test {

static auto BuildMiniTouchClient() {
    MiniTouchClient::StartMiniTouchService(
        "E:/Program Files/Netease/MuMu Player 12", 16384, 3912);
    return MiniTouchClient("127.0.0.1", 3912);
}

static auto BuildMumuClient() {
    return MumuClient("E:/Program Files/Netease/MuMu Player 12", 0, "default");
}

static void TestScreen(IScreen &screen) {
    Frame frame = screen.GetFrame();
    cv::imshow("Mumu Screen", frame.img);
    cv::waitKey(0);
}

static void TestTouch() {
    auto mumu = BuildMumuClient();
    TouchController touch(mumu);
    TouchExecutor executor = touch.CreateExecutor();
    for (int i = 0; i < 10; ++i) {
        executor.TouchTap(GetCurrentTimeMs() + i * 1000,
                          cv::Point{300, 100 + i * 100}, 10 * 1000);
    }
    executor.Start();
    executor.Shutdown(false);
}

} // namespace psh::test

#endif // !PSH_TEST_PSH_TEST_HPP_
