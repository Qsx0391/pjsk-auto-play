#include <QtWidgets/QApplication>

#include "spdlog/spdlog.h"

#include "mini_touch_client.h"
#include "mumu_client.h"
#include "auto_player.h"

#include "main_window.h"

namespace psh::test {

static auto BuildMiniTouchClient() {
    MiniTouchClient::StartMiniTouchService(
        "E:/Program Files/Netease/MuMu Player 12", 16384, 3912);
    return MiniTouchClient("127.0.0.1", 3912);
}

static void TestTouch(ITouch &touch) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    touch.TouchDown({500, 500}, 1);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    touch.TouchUp(1);
    for (int i = 0; i < 5; ++i) {
        touch.TouchTap({500, 500}, 1, 20);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i < 5; ++i) {
        touch.TouchSlideAsyn({100, 300}, {600, 300}, 1, 100);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

static auto BuildMumuClient() {
    return MumuClient("E:/Program Files/Netease/MuMu Player 12", 0, "default");
}

static void TestScreen(IScreen &screen) {
    cv::Mat img = screen.Screencap();
    cv::imshow("Mumu Screen", img);
    cv::waitKey(0);
}

//static auto BuildAutoPlayer(MumuClient &mumu, MiniTouchClient &mini_touch) {
//    return AutoPlayer({mumu, {1, 2, 3, 4}}, {mumu, {5, 6, 7, 8, 9, 10}},
//                      {mini_touch, {0, 1, 2, 3, 4, 5}}, mumu,
//                      AutoPlayer::kDefaultTrackConfig,
//                      AutoPlayer::kDefaultPlayConfig);
//}

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
    // auto player = psh::test::BuildAutoPlayer(mumu, mumu, mumu);

    return StartQt(argc, argv);
    // psh::test::TestTouch(mumu);
    // psh::test::TestScreen(mumu);
    // player.TestCheckLine();
    // player.TestTouch();
    // psh::test::TestAutoPlayer(player);
    // return 0;
}
