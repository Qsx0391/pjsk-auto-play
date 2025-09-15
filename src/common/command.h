#pragma once

#ifndef COMMON_COMMAND_H_
#define COMMON_COMMAND_H_

#include <qstring.h>
#include <QProcess>

namespace psh {

QString ExecCommand(const QString& program, const QStringList& cmd);

}

#endif // !COMMON_COMMAND_H_
