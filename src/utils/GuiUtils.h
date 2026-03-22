#ifndef GUIUTILS_H
#define GUIUTILS_H

#include <QFuture>
#include <QMessageBox>

#include "Utils.h"

namespace utils {

template <typename T> bool checkFutureExceptionAndWarn(QFuture<T> future)
{
    try {
        future.waitForFinished();
        return true;
    } catch (...) {
        auto msg = exceptionToString(std::current_exception());
        QMessageBox::critical(nullptr, {}, msg);
        return false;
    }
}

} // namespace utils

#endif // GUIUTILS_H
