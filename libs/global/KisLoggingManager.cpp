/*
 * Copyright (c) 2017 Alvin Wong <alvinhochun@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "KisLoggingManager.h"

#include <QDateTime>
#include <QFile>
#include <QLoggingCategory>
#include <QSet>
#include <QTextStream>

#include <memory>

using ScopedLogCapturer = KisLoggingManager::ScopedLogCapturer;

namespace
{
    QtMessageHandler oldMessageHandler;
    QLoggingCategory::CategoryFilter oldCategoryFilter;

    QSet<const ScopedLogCapturer *> capturerSet;
    std::unique_ptr<QFile> logFile;
    std::unique_ptr<QTextStream> logFileWriter;

    const char *msgTypeToText(QtMsgType type) {
        switch (type) {
        case QtDebugMsg:
            return "debug";
        case QtInfoMsg:
            return "info";
        case QtWarningMsg:
            return "warning";
        case QtCriticalMsg:
            return "critical";
        case QtFatalMsg:
            return "fatal";
        default:
            return "unknown";
        }
    }
} // namespace

class KisLoggingManager::Private
{
    friend class KisLoggingManager;

    static void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        // Log to file
        if (logFileWriter) {
            *logFileWriter << QDateTime::currentDateTime().toString(Qt::ISODate) << ' ' << context.category << '.' << msgTypeToText(type) << '\t' << msg << '\n';
            logFileWriter->flush();
        }

        Q_FOREACH (const ScopedLogCapturer *const &capturer, capturerSet) {
            if (capturer->m_category == context.category) {
                capturer->m_callback(type, context, msg);
            }
        }
        // TODO: Hide capture-only messages from default output
        oldMessageHandler(type, context, msg);
    }

    static void myCategoryFilter(QLoggingCategory *category)
    {
        oldCategoryFilter(category);
        // Enable categories to be captured
        // TODO: Keep track of default filter stage to hide message from output
        Q_FOREACH (const ScopedLogCapturer *const &capturer, capturerSet) {
            if (capturer->m_category == category->categoryName()) {
                category->setEnabled(QtDebugMsg, true);
                category->setEnabled(QtInfoMsg, true);
                category->setEnabled(QtWarningMsg, true);
                category->setEnabled(QtCriticalMsg, true);
            }
        }
    }

    static void refreshCategoryFilter()
    {
        QLoggingCategory::installFilter(myCategoryFilter);
    }
}; // class KisLoggingManager::Private

void KisLoggingManager::initialize()
{
    // Install our QtMessageHandler for capturing logging messages
    oldMessageHandler = qInstallMessageHandler(KisLoggingManager::Private::myMessageHandler);
    // HACK: Gets the default CategoryFilter because the filter function may
    //       be called synchronously.
    oldCategoryFilter = QLoggingCategory::installFilter(nullptr);
    // Install our CategoryFilter for filtering
    KisLoggingManager::Private::refreshCategoryFilter();
}

void KisLoggingManager::initializeLogFile(QString path)
{
    logFileWriter.reset();
    logFile.reset(new QFile(path));
    if (!logFile->open(QFile::WriteOnly)) {
        logFile.reset();
        return;
    }
    logFileWriter.reset(new QTextStream(logFile.get()));
}

ScopedLogCapturer::ScopedLogCapturer(QByteArray category, ScopedLogCapturer::callback_t callback)
    : m_category(category)
    , m_callback(callback)
{
    capturerSet.insert(this);
    KisLoggingManager::Private::refreshCategoryFilter();
}

ScopedLogCapturer::~ScopedLogCapturer()
{
    capturerSet.remove(this);
    KisLoggingManager::Private::refreshCategoryFilter();
}
