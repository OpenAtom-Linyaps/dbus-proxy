/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "post_thread.h"

#include <QFile>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

// 存储dbus信息服务器配置文件
const QString serverConfigPath = "/deepin/linglong/config/dbus_proxy_config";

PostThread::PostThread(const QString &id, const QString &busName, const QString &busPath, const QString &busPathIfce,
                       QThread *pThread, QObject *parent)
    : QObject(parent)
    , appId(id)
    , name(busName)
    , path(busPath)
    , interface(busPathIfce)
    , thread(pThread)
{
}

/*
 * 将应用访问的dbus信息发送到服务端
 */
void PostThread::sendDataToServer()
{
    if (name.isEmpty() && path.isEmpty() && interface.isEmpty()) {
        emit finishPost(thread, this);
        return;
    }

    QFile dbFile(serverConfigPath);
    auto ret = dbFile.open(QIODevice::ReadOnly);
    if (!ret) {
        qWarning() << "open config file err";
        emit finishPost(thread, this);
        return;
    }
    QString qValue = dbFile.readAll();
    dbFile.close();
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(qValue.toUtf8(), &parseJsonErr);
    if (QJsonParseError::NoError != parseJsonErr.error) {
        qWarning() << "parse config file err";
        emit finishPost(thread, this);
        return;
    }
    QJsonObject dataObject = document.object();
    if (!dataObject.contains("dbusDbUrl")) {
        qDebug() << "dbusDbUrl not found in config";
        emit finishPost(thread, this);
        return;
    }
    const QString configValue = dataObject["dbusDbUrl"].toString();
    const QUrl url(configValue + "/apps/adddbusproxy");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject obj;
    obj["appId"] = appId;
    obj["name"] = name;
    obj["path"] = path;
    obj["interface"] = interface;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();
    QNetworkAccessManager mgr;
    qDebug() << "begin to send data to test server:" + url.toString();
    QNetworkReply *reply = mgr.post(request, data);
    QString responseData;
    QEventLoop eventLoop;
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, [&]() {
        if (reply->error() == QNetworkReply::NoError) {
            responseData = QString::fromUtf8(reply->readAll());
            qDebug() << "receive data from server:" << responseData;
        } else {
            QString err = reply->errorString();
            qCritical() << err;
        }
        reply->deleteLater();
        eventLoop.quit();
    });
    // 1s 超时
    QTimer::singleShot(1000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    qDebug() << "send data to test server:";
    qDebug().noquote() << data;
    emit finishPost(thread, this);
}