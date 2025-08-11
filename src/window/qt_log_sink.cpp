#include "qt_log_sink.h"
#include "main_window.h"
#include <spdlog/details/log_msg.h>
#include <QMetaObject>

namespace psh {

QtLogSink::QtLogSink(MainWindow* main_window) : main_window_(main_window) {
    // 连接信号到主窗口的槽函数
    connect(this, &QtLogSink::logMessage, main_window_, &MainWindow::AddLogMessage, Qt::QueuedConnection);
}

void QtLogSink::sink_it_(const spdlog::details::log_msg& msg) {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
    
    QString message = QString::fromUtf8(formatted.data(), static_cast<int>(formatted.size()));
    
    emit logMessage(message);
}

void QtLogSink::flush_() {

}

} // namespace psh
