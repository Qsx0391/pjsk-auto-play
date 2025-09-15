#include <QtWidgets/QApplication>

#include "spdlog/spdlog.h"

#include "window/main_window.h"
#include "test/psh_test.hpp"

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
    // psh::test::TestTouch();
}
