/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <QDebug>
#include <QDir>

#include "proxy/dbus_proxy.h"

TEST(dbusProxy, proxy01)
{
    QString daemonPath = QString("/run/user/%1/bus").arg(getuid());
    DbusProxy server;
    server.saveDbusDaemonPath(daemonPath);

    QLocalSocket *proxyClient = new QLocalSocket();
    bool ret = server.startConnectDbusDaemon(proxyClient, daemonPath);
    delete proxyClient;
    EXPECT_EQ(ret, true);

    const QString appId = "org.deepin.music";
    server.saveAppId(appId);
    const QString socketPath = QDir::currentPath() + "/listen_socket";
    ret = server.startListenBoxClient(socketPath);
    EXPECT_EQ(ret, true);
}
