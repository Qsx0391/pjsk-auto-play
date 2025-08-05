#include <QtWidgets/QApplication>

#include "spdlog/spdlog.h"

#include "mini_touch_client.h"
#include "mumu_client.h"
#include "auto_player.h"
#include "note_time_estimator.h"

#include "main_window.h"

namespace psh::test {

static auto BuildMiniTouchClient() {
    MiniTouchClient::StartMiniTouchService(
        "E:/Program Files/Netease/MuMu Player 12", 16384, 3912);
    return MiniTouchClient("127.0.0.1", 3912);
}

static auto BuildAutoPlayer(MumuClient &mumu, NoteTimeEstimator &estimator) {
    return AutoPlayer(mumu, mumu, mumu, mumu, estimator, kDefaultTrackConfig,
                      kDefaultPlayConfig);
}

static auto BuildMumuClient() {
    return MumuClient("E:/Program Files/Netease/MuMu Player 12", 0, "default");
}

static auto BuildNoteFinder() { return NoteFinder(); }

static auto BuildNoteTimeEstimator(IScreen &screen, NoteFinder &finder) {
    return NoteTimeEstimator(kDefaultTrackConfig.check_upper_y,
                             kDefaultTrackConfig.check_lower_y);
}

static void TestTouch(ITouch &touch) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i < 10; ++i) {
        touch.TouchTapAsyn({100, 100}, 20, 500);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i < 10; ++i) {
        touch.TouchSlideAsyn({100, 100}, {600, 100}, 100, 5, 1.0, 1.0, 500);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

static void TestScreen(IScreen &screen) {
    cv::Mat img = screen.Screencap();
    cv::imshow("Mumu Screen", img);
    cv::waitKey(0);
}

static void TestAutoPlayer(AutoPlayer &player) {
    player.Start();
    std::cin.get();
    player.Stop();
}

} // namespace psh::test

static int StartQt(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}

int main(int argc, char *argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    spdlog::set_level(spdlog::level::debug);
    // auto minitouch = psh::test::BuildMiniTouchClient();
    // auto mumu = psh::test::BuildMumuClient();
    // auto player = psh::test::BuildAutoPlayer(mumu);
    // auto finder = psh::test::BuildNoteFinder();
    // auto estimator = psh::test::BuildNoteTimeEstimator(mumu, finder);
    // psh::test::TestTouch(mumu);

    return StartQt(argc, argv);
    // return 0;
}
