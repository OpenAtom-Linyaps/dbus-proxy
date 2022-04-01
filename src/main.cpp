/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <signal.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <dbus/dbus.h>

#include <QCoreApplication>
#include <QDebug>

#include "filter/dbus_filter.h"
#include "proxy/dbus_proxy.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss.zzz} [%{appname}] [%{type}] %{message}");

    if (argc < 7) {
        qCritical() << "dbus proxy param err";
        return -1;
    }

    // socket文件未删除，使用ll-box 将同一个json文件作为沙箱配置时会有问题（需要手动改socketpath地址）
    // unlink(argv[3]);
    QString socketPath = argv[3];
    if (socketPath.isEmpty()) {
        qCritical() << "dbus proxy socketPath err";
        return -1;
    }

    qInfo() << "dbus proxy socketPath:" << socketPath;

    QString daemonPath = "";
    if (strcmp(argv[2], "session") == 0) {
        daemonPath = QString("/run/user/%1/bus").arg(getuid());
    } else if (strcmp(argv[2], "system") == 0) {
        daemonPath = "/run/dbus/system_bus_socket";
    } else {
        qCritical() << "user input dbus type err";
        return -1;
    }
    qInfo() << "dbus proxy daemonPath:" << daemonPath;

    DbusProxy server;
    // server.saveBoxSocketPath(socketPath);
    server.saveDbusDaemonPath(daemonPath);

    // 保存应用的appId 向权限模块申请授权时使用
    server.saveAppId(QString(argv[1]));

    server.filter.addNameFilter("org.freedesktop.portal.*");
    server.filter.addPathFilter("/org/freedesktop/portal/*");
    server.filter.addInterfaceFilter("org.freedesktop.portal.");

    server.filter.addNameFilter("org.freedesktop.DBus");
    server.filter.addPathFilter("/");
    server.filter.addPathFilter("/org/freedesktop/DBus");
    server.filter.addInterfaceFilter("org.freedesktop.DBus");

    // 初始化filter
    QStringList nameFilterList = QString(QLatin1String(argv[4])).split(",");
    for (auto item : nameFilterList) {
        server.filter.addNameFilter(item);
    }
    QStringList pathFilterList = QString(QLatin1String(argv[5])).split(",");
    for (auto item : pathFilterList) {
        server.filter.addPathFilter(item);
    }
    QStringList interfaceFilterList = QString(QLatin1String(argv[6])).split(",");
    for (auto item : interfaceFilterList) {
        server.filter.addInterfaceFilter(item);
    }

    prctl(PR_SET_PDEATHSIG, SIGKILL);

    QString config = "";
    server.filter.dumpConfig(config);
    server.startListenBoxClient(socketPath);
    return app.exec();
}