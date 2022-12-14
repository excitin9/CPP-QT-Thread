/*
    Copyright (c) 2009-10 Qtrac Ltd. All rights reserved.

    This program or module is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version. It is provided
    for educational purposes and is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
    the GNU General Public License for more details.
*/

#include "convertimagetask.hpp"
#include <QDir>
#include <QFileInfo>
#include <QImage>

//QRunnable不是QObject的子类，不会内置支持信号和槽
//私有类 阻止类的子类化
//但也阻止了run()在实例中的调用
void ConvertImageTask::run()
{
    foreach (const QString &source, m_sourceFiles)
    {
        if (*m_stopped)
            return;
        QImage image(source);
        QString target(source);
        target.chop(QFileInfo(source).suffix().length());
        target += m_targetType;
        if (*m_stopped)
            return;
        bool saved = image.save(target);

        QString message = saved
                ? QObject::tr("Saved '%1'")
                              .arg(QDir::toNativeSeparators(target))
                : QObject::tr("Failed to convert '%1'")
                              .arg(QDir::toNativeSeparators(source));
        //QMetaObject启动一个槽，每个参数都必须使用Q_ARG宏
        QMetaObject::invokeMethod(m_receiver, "announceProgress",
                Qt::QueuedConnection, Q_ARG(bool, saved),
                Q_ARG(QString, message));
    }
}
