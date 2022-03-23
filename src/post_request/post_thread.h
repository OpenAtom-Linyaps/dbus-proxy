
/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QObject>

class PostThread : public QObject
{
    Q_OBJECT

public slots:
    void sendDataToServer();

public:
    PostThread(const QString &id, const QString &busName, const QString &busPath, const QString &busPathIfce, QObject *parent = 0);

private:
    QString appId;
    QString name;
    QString path;
    QString interface;
};
