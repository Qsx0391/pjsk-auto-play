#include <QtWidgets/QApplication>

#include "spdlog/spdlog.h"

#include "mini_touch_client.h"
#include "mumu_client.h"
#include "auto_player.h"
#include "note_time_estimator.h"

#include "main_window.h"

static int StartQt(int argc, char *argv[]) {
    QApplication app(argc, argv);
    psh::MainWindow window;
    window.show();
    return app.exec();
}

int main(int argc, char *argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    // spdlog::set_level(spdlog::level::debug);

    return StartQt(argc, argv);
}
