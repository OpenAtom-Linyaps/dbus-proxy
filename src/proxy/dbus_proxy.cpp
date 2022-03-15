/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dbus_proxy.h"

#include <unistd.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include <QEventLoop>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

DbusProxy::DbusProxy()
    : serverProxy(new QLocalServer()), clientProxy(new QLocalSocket()), isConnectDbusDaemon(false)
{
    // dbus-proxy server, wait for dbus client in box to connect
    // serverProxy = new QLocalServer();
    // dbus client, be used to connect to the dbus daemon
    // clientProxy = new QLocalSocket();
    connect(serverProxy.get(), SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

DbusProxy::~DbusProxy()
{
    if (serverProxy) {
        serverProxy->close();
        // delete serverProxy;
    }
    if (clientProxy) {
        clientProxy->close();
        // delete clientProxy;
    }
}

/*
 * 启动监听
 *
 * @param socketPath: socket监听地址
 *
 * @return bool: true:成功 其它:失败
 */
bool DbusProxy::startListenBoxClient(const QString &socketPath)
{
    if (socketPath.isEmpty()) {
        qCritical() << "socketPath not exist";
        return false;
    }
    QLocalServer::removeServer(socketPath);
    serverProxy->setSocketOptions(QLocalServer::UserAccessOption);
    bool ret = serverProxy->listen(socketPath);
    if (!ret) {
        qCritical() << "listen box dbus client error";
        return false;
    }
    qInfo() << "startListenBoxClient ret:" << ret;
    return ret;
}

/*
 * 连接dbus-daemon
 *
 * @param daemonPath: dbus-daemon地址
 *
 * @return bool: true:成功 其它:失败
 */
bool DbusProxy::startConnectDbusDaemon(const QString &daemonPath)
{
    if (daemonPath.isEmpty()) {
        qCritical() << "daemonPath is empty";
        return false;
    }
    // bind clientProxy to dbus daemon
    connect(clientProxy.get(), SIGNAL(connected()), this, SLOT(onConnectedServer()));
    connect(clientProxy.get(), SIGNAL(disconnected()), this, SLOT(onDisconnectedServer()));
    connect(clientProxy.get(), SIGNAL(readyRead()), this, SLOT(onReadyReadServer()));
    qInfo() << "start connect dbus-daemon...";
    clientProxy->connectToServer(daemonPath);
    // 等待3s
    if (!clientProxy->waitForConnected(3000)) {
        qCritical() << "connect dbus-daemon error, msg:" << clientProxy->errorString();
        return false;
    }
    return true;
}

void DbusProxy::onNewConnection()
{
    // 沙箱应用客户端多次dbus调用没有主动断开连接
    if (clientProxy) {
        clientProxy->disconnectFromServer();
        qInfo() << "disconnectFromServer before onNewConnection";
    }
    qInfo() << "onNewConnection called, server: " << serverProxy->serverName();
    QLocalSocket *client = serverProxy->nextPendingConnection();
    connect(client, SIGNAL(readyRead()), this, SLOT(onReadyReadClient()));
    connect(client, SIGNAL(disconnected()), this, SLOT(onDisconnectedClient()));
}


int requestPermission(const QString &appId)
{
    QDBusInterface interface("org.desktopspec.permission", "/org/desktopspec/permission", "org.desktopspec.permission",
                             QDBusConnection::sessionBus());
    // 25s dbus 客户端默认25s必须回
    interface.setTimeout(1000 * 60 * 25);
    QDBusPendingReply<int> reply = interface.call("request", appId, "org.desktopspec.permission.Account");
    reply.waitForFinished();
    int ret = -1;
    if (reply.isValid()) {
        ret = reply.value();
    }
    qInfo() << "requestPermission ret:" << ret;
    return ret;
}

void DbusProxy::onReadyReadClient()
{
    // box client socket address
    boxClient = static_cast<QLocalSocket *>(sender());
    if (boxClient) {
        QByteArray data = boxClient->readAll();
        qInfo() << "Read Data From Client size:" << data.size();
        qInfo() << "Read Data From Client:" << data;
        QByteArray helloData;
        bool isHelloMsg = data.contains("BEGIN");
        if (isHelloMsg) {
            // auth begin msg is not normal header
            qWarning() << "get client hello msg";
            helloData = data.mid(7);
        }

        Header header;
        bool ret = false;
        if (isHelloMsg) {
            ret = parseHeader(helloData, &header);
        } else {
            ret = parseHeader(data, &header);
        }
        if (!ret) {
            qWarning() << "parseHeader is not a normal msg";
        }
        if (!isConnectDbusDaemon) {
            ret = startConnectDbusDaemon(daemonPath);
            qInfo() << "start reconnect dbus-daemon ret:" << ret;
        }

        // 判断是否满足过滤规则
        bool isMatch = filter.isMessageMatch(header.destination, header.path, header.interface);
        qInfo() << "msg destination:" << header.destination << ", header.path:" << header.path
                << ", header.interface:" << header.interface << ", dbus msg match filter ret:" << isMatch;
        // 握手信息不拦截
        if (!isDbusAuthMsg(data) && !isMatch) {
            // 未配置权限申请用户授权
            int result = Allow;
            if (!qgetenv("DBUS_PROXY_INTERCEPT").isNull()) {
                result = requestPermission(appId);
            }
            // 记录应用通过dbus访问的宿主机资源
            if (result != Allow) {
                if (isNeedReply(&header)) {
                    QByteArray reply = createFakeReplyMsg(
                        data, header.serial + 1, boxClientAddr, "org.freedesktop.DBus.Error.AccessDenied",
                        "org.freedesktop.DBus.Error.AccessDenied, please config permission first!");
                    // 伪造 错误消息格式给客户端
                    // 将消息发送方 header中的serial 填充到 reply_serial
                    // 填写消息类型 flags(是否需要回复) 消息body 需要修改消息body长度
                    // 生成一个惟一的序列号
                    boxClient->write(reply);
                    boxClient->waitForBytesWritten(3000);
                    qInfo() << "reply size:" << reply.size();
                    qInfo() << reply;
                }
                return;
            }
        }
        if (isConnectDbusDaemon) {
            clientProxy->write(data);
            clientProxy->waitForBytesWritten(3000);
            qInfo() << "send data to dbus-daemon done";
        }
    }
}

void DbusProxy::onDisconnectedClient()
{
    QLocalSocket *sender = static_cast<QLocalSocket *>(QObject::sender());
    if (sender) {
        sender->disconnectFromServer();
    }
    qInfo() << "onDisconnectedClient called sender:" << sender;
    // box 客户端断开连接时，断开代理与dbus daemon的连接
    if (clientProxy) {
        clientProxy->disconnectFromServer();
    }
}

// dbus-daemon 服务端回调函数
void DbusProxy::onConnectedServer()
{
    qInfo() << "connected to dbus-daemon:" << clientProxy->fullServerName() << " success";
    isConnectDbusDaemon = true;
}

void DbusProxy::onReadyReadServer()
{
    QByteArray receiveDta = clientProxy->readAll();
    qInfo() << "receive from dbus-daemon, data size:" << receiveDta.size();
    qInfo() << receiveDta;
    // is a right way to judge?
    bool isHelloReply = receiveDta.contains("NameAcquired");
    if (isHelloReply) {
        qInfo() << "parse msg header from dbus-daemon";
        Header header;
        bool ret = parseHeader(receiveDta, &header);
        if (!ret) {
            qCritical() << "dbus-daemon msg parseHeader err";
        }
        boxClientAddr = header.destination;
        qInfo() << "boxClientAddr:" << boxClientAddr;
    }

    // 将消息转发给客户端
    if (boxClient) {
        boxClient->write(receiveDta);
        // boxClient->flush();
        boxClient->waitForBytesWritten(3000);
        qInfo() << "send data to box dbus client done,data size:" << receiveDta.size();
    }
}

// 与dbus-daemon 断开连接
void DbusProxy::onDisconnectedServer()
{
    QLocalSocket *sender = static_cast<QLocalSocket *>(QObject::sender());
    if (sender) {
        sender->disconnectFromServer();
    }
    isConnectDbusDaemon = false;
    qInfo() << "onDisconnectedServer called sender:" << sender;
}

QByteArray DbusProxy::createFakeReplyMsg(const QByteArray &byteMsg, quint32 serial, const QString &dst,
                                         const QString &errorName, const QString &errorMsg)
{
    DBusError dbErr;
    dbus_error_init(&dbErr);
    DBusMessage *receiveMsg = dbus_message_demarshal(byteMsg.constData(), byteMsg.size(), &dbErr);
    if (!receiveMsg) {
        qCritical() << "dbus_message_demarshal failed";
        if (dbus_error_is_set(&dbErr)) {
            qCritical() << "dbus_message_demarshal err msg:" << dbErr.message;
            dbus_error_free(&dbErr);
        }
        return nullptr;
    }

    std::string nameString = errorName.toStdString();
    std::string msgString = errorMsg.toStdString();
    DBusMessage *reply = dbus_message_new_error(receiveMsg, nameString.c_str(), msgString.c_str());
    std::string destination = dst.toStdString();
    auto ret = dbus_message_set_destination(reply, destination.c_str());
    if (!ret) {
        // dbus_message_unref(receiveMsg);
        qCritical() << "createFakeReplyMsg set destination failed";
    }
    dbus_message_set_serial(reply, serial);

    char *replyAsc;
    int len = 0;
    ret = dbus_message_marshal(reply, &replyAsc, &len);
    if (!ret) {
        // dbus_message_unref(receiveMsg);
        qCritical() << "createFakeReplyMsg dbus_message_marshal failed";
    }
    QByteArray data(replyAsc, len);
    dbus_free(replyAsc);
    dbus_message_unref(receiveMsg);
    dbus_message_unref(reply);
    return data;
}