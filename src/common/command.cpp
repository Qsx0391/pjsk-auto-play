#include "common/command.h"

#include <spdlog/spdlog.h>

namespace psh {

QString ExecCommand(const QString& program, const QStringList& cmd) {
    QProcess p;
    p.setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments* args) {
            args->flags |= CREATE_NO_WINDOW;
        });
    p.start(program, QStringList{cmd});
    if (!p.waitForFinished()) {
        auto s = p.errorString();
        spdlog::error("QProcess error: {}",
                      p.errorString().toUtf8().constData());
        throw std::runtime_error("QProcess timeout");
    }
    QByteArray stdOut = p.readAllStandardOutput();
    QByteArray stdErr = p.readAllStandardError();
    return QString::fromLocal8Bit(stdOut + stdErr);
}

} // namespace psh