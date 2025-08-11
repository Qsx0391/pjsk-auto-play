#pragma once

#ifndef QT_LOG_SINK_H_
#define QT_LOG_SINK_H_

#include <mutex>

#include <QObject>
#include <QString>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>

namespace psh {

class MainWindow;

class QtLogSink : public QObject, public spdlog::sinks::base_sink<std::mutex> {
    Q_OBJECT

public:
    explicit QtLogSink(MainWindow* main_window);

signals:
    void logMessage(const QString& message);

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override;

private:
    MainWindow* main_window_;
};

} // namespace psh

#endif // !QT_LOG_SINK_H_
