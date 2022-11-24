/*
 * SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.  
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "dbus_proxy.h"

#include <unistd.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

DbusProxy::DbusProxy()
    : serverProxy(new QLocalServer())
{
    connect(serverProxy.get(), SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

DbusProxy::~DbusProxy()
{
    if (serverProxy) {
        serverProxy->close();
    }

    for (const auto &client : relations.keys()) {
        if (relations[client]) {
            delete relations[client];
            client->close();
        }
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
    qDebug() << "startListenBoxClient ret:" << ret;
    return ret;
}

/*
 * 连接dbus-daemon
 *
 * @param localProxy: 请求与dbus-daemon连接的客户端
 * @param daemonPath: dbus-daemon地址
 *
 * @return bool: true:成功 其它:失败
 */
bool DbusProxy::startConnectDbusDaemon(QLocalSocket *localProxy, const QString &daemonPath)
{
    if (daemonPath.isEmpty()) {
        qCritical() << "daemonPath is empty";
        return false;
    }
    // bind clientProxy to dbus daemon
    connect(localProxy, SIGNAL(connected()), this, SLOT(onConnectedServer()));
    connect(localProxy, SIGNAL(disconnected()), this, SLOT(onDisconnectedServer()));
    connect(localProxy, SIGNAL(readyRead()), this, SLOT(onReadyReadServer()));
    qDebug() << "proxy client:" << localProxy << " start connect dbus-daemon...";
    localProxy->connectToServer(daemonPath);
    // 等待代理连接dbus-daemon
    if (!localProxy->waitForConnected(3000)) {
        qCritical() << "connect dbus-daemon error, msg:" << localProxy->errorString();
        return false;
    }
    qDebug() << "startConnectDbusDaemon done";
    return true;
}

void DbusProxy::onNewConnection()
{
    QLocalSocket *client = serverProxy->nextPendingConnection();
    qDebug() << "onNewConnection called, client:" << client;
    connect(client, SIGNAL(readyRead()), this, SLOT(onReadyReadClient()));
    connect(client, SIGNAL(disconnected()), this, SLOT(onDisconnectedClient()));

    QLocalSocket *proxyClient = new QLocalSocket();
    bool ret = startConnectDbusDaemon(proxyClient, daemonPath);
    relations.insert(client, proxyClient);
    qDebug() << "onNewConnection create: " << client << "<===>" << proxyClient << " relation, ret:" << ret;
}

int DbusProxy::requestPermission(const QString &appId, const QString &id)
{
    if (id.isEmpty()) {
        qCritical() << "id is empty";
        return -1;
    }
    QDBusInterface interface("org.desktopspec.permission", "/org/desktopspec/permission", "org.desktopspec.permission",
                             QDBusConnection::sessionBus());
    QDBusPendingReply<QString> reply = interface.call("Request", appId, "linglong", id);
    reply.waitForFinished();
    int ret = -1;
    if (reply.isValid()) {
        ret = reply.value().toInt();
        // DDE 查询到用户上次弹窗选择结果是拒绝则返回1
        if (1 == ret) {
            QDBusPendingReply<void> dialogReply = interface.call("ShowDisablePermissionDialog", appId, "linglong", id);
            dialogReply.waitForFinished();
        }
    } else {
        if ("org.desktopspec.permission.SystemLevelRestrictions" == reply.error().name()) {
            QDBusPendingReply<void> dialogReply = interface.call("ShowDisablePermissionDialog", appId, "linglong", id);
            dialogReply.waitForFinished();
        }
        qCritical() << appId << " requestPermission err:" << reply.error();
    }
    qDebug() << appId << " requestPermission id:" << id << ",ret:" << ret;
    return ret;
}

QString DbusProxy::getPermissionId(const QString &name, const QString &path, const QString &ifce)
{
    const QString cfgPath = "/usr/share/permission/policy/linglong/dbus_map_config";
    QFile cfgFile(cfgPath);
    if (!cfgFile.open(QIODevice::ReadOnly)) {
        qCritical() << "getPermissionId err" << cfgFile.errorString();
        return "";
    }
    QString qValue = cfgFile.readAll();
    cfgFile.close();
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(qValue.toUtf8(), &parseJsonErr);
    if (QJsonParseError::NoError != parseJsonErr.error) {
        qCritical() << "getPermissionId parse config file err";
        return "";
    }

    QJsonObject dataObject = document.object();
    // 根据dbus信息查找权限id
    for (const auto &key : dataObject.keys()) {
        auto dbusObject = dataObject.value(key);
        if (dbusObject.isArray()) {
            QJsonArray dbusArray = dbusObject.toArray();
            for (int i = 0; i < dbusArray.size(); i++) {
                QJsonObject item = dbusArray.at(i).toObject();
                if (name == item.value("name").toString() && path == item.value("path").toString()
                    && ifce == item.value("ifce").toString()) {
                    return key;
                }
            }
        }
    }
    qWarning() << "permission id not found " << QString("name:%1,path:%2,interface:%3").arg(name).arg(path).arg(ifce);
    return "";
}

void DbusProxy::onReadyReadClient()
{
    // box client socket address
    QLocalSocket *boxClient = static_cast<QLocalSocket *>(sender());
    qDebug() << boxClient << "onReadyReadClient called";

    if (boxClient) {
        // 查找客户端对应的代理
        QLocalSocket *proxyClient = nullptr;
        if (relations.contains(boxClient)) {
            proxyClient = relations[boxClient];
        } else {
            qCritical() << "boxClient:" << boxClient << " related proxyClient not found";
        }
        // 代理未连接上dbus daemon，尝试重新连接dbus daemon一次
        if (proxyClient && !connStatus.contains(proxyClient)) {
            bool ret = startConnectDbusDaemon(proxyClient, daemonPath);
            qDebug() << proxyClient << " start reconnect dbus-daemon ret:" << ret;
        }

        // 客户端在代理处理转发期间又发来了数据不会触发onReadyReadClient回调
        while (boxClient->bytesAvailable() > 0) {
            // dbus 消息会缓存 需要分割成一条一条的
            QByteArray data = boxClient->readAll();
            qDebug() << "Read Data From Client size:" << data.size();
            qDebug() << "Read Data From Client:" << data;
            QList<QByteArray> msgList;
            // 分割缓存中的dbus消息
            splitDBusMsg(data, msgList);
            for (auto item : msgList) {
                Header header;
                bool isMatch = false;
                if (!isDbusAuthMsg(item)) {
                    if (!parseDBusMsg(item, &header)) {
                        qWarning() << "onReadyReadClient parse an abnormal dbus msg, msg:" << item
                                   << ", size:" << item.size();
                    } else {
                        // 判断是否满足过滤规则 当前实现由白名单改为黑名单
                        isMatch = filter.isMessageMatch(header.destination, header.path, header.interface);
                        qDebug() << "dbus msg serial:" << header.serial << ", reply_serial:" << header.replySerial
                                 << ", sender:" << header.sender << ", destination:" << header.destination
                                 << ", header.path:" << header.path
                                 << ", header.interface:" << header.interface << ", header.member:" << header.member
                                 << ", dbus msg match filter ret:" << isMatch;
                    }
                }

                // 握手信息不拦截
                if (!isDbusAuthMsg(item) && isMatch) {
                    // 未配置权限申请用户授权
                    int result = Allow;
                    if (!qgetenv("DBUS_PROXY_INTERCEPT").isNull()) {
                        QString id = getPermissionId(header.destination, header.path, header.interface);
                        result = requestPermission(appId, id);
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
                            boxClient->waitForBytesWritten(1000);
                            qDebug() << "reply size:" << reply.size();
                            qDebug() << reply;
                        }
                        return;
                    }
                }
                if (!connStatus.contains(proxyClient)) {
                    qCritical() << proxyClient << " not connect to dbus-daemon";
                    return;
                }
                proxyClient->write(item);
                proxyClient->waitForBytesWritten(1000);
                qDebug() << proxyClient << " send data to dbus-daemon done, msg:" << item << ", size:" << item.size();
                // Fix 客户端在代理处理转发期间又发来了数据不会触发回调
                // while (boxClient->bytesAvailable() > 0) {
                //     QByteArray leftData = boxClient->readAll();
                //     proxyClient->write(leftData);
                //     proxyClient->waitForBytesWritten(1000);
                //     qDebug() << boxClient << " onReadyReadClient data left:" << boxClient->bytesAvailable();
                // }
            }
        }
    }
}

void DbusProxy::onDisconnectedClient()
{
    QLocalSocket *sender = static_cast<QLocalSocket *>(QObject::sender());
    if (sender) {
        sender->disconnectFromServer();
    }
    qDebug() << "onDisconnectedClient called, sender:" << sender;
    QLocalSocket *proxyClient = relations[sender];
    // box 客户端断开连接时，断开代理与dbus daemon的连接
    if (!proxyClient) {
        qCritical() << "onDisconnectedClient box client: " << sender << " related proxyClient not found";
        return;
    }
    proxyClient->disconnectFromServer();
    relations.remove(sender);
    proxyClient->deleteLater();
}

// dbus-daemon 服务端回调函数
void DbusProxy::onConnectedServer()
{
    QLocalSocket *proxyClient = static_cast<QLocalSocket *>(QObject::sender());
    qDebug() << proxyClient << " connected to dbus-daemon success";
    connStatus.insert(proxyClient, true);
}

void DbusProxy::onReadyReadServer()
{
    QLocalSocket *daemonClient = static_cast<QLocalSocket *>(QObject::sender());
    // 查找代理对应的客户端
    QLocalSocket *boxClient = nullptr;
    for (const auto &client : relations.keys()) {
        if (relations[client] == daemonClient) {
            boxClient = client;
            break;
        }
    }

    while (daemonClient->bytesAvailable() > 0) {
        QByteArray receiveDta = daemonClient->readAll();
        qDebug() << "receive from dbus-daemon, data size:" << receiveDta.size();
        qDebug() << receiveDta;
        QList<QByteArray> msgList;
        // 分割缓存中的dbus消息
        splitDBusMsg(receiveDta, msgList);
        for (const auto &item : msgList) {
            // is a right way to judge?
            bool isHelloReply = item.contains("NameAcquired");
            if (isHelloReply) {
                qDebug() << "parse msg header from dbus-daemon";
                Header header;
                if (!parseDBusMsg(item, &header)) {
                    qWarning() << "onReadyReadServer parse an abnormal dbus msg, msg:" << item
                               << ", size:" << item.size();
                }
                boxClientAddr = header.destination;
                qDebug() << "boxClientAddr:" << boxClientAddr;
            }
            // 将消息转发给客户端
            if (boxClient) {
                boxClient->write(item);
                boxClient->waitForBytesWritten(1000);
                qDebug() << boxClient << " send data to box dbus client done, msg:" << item << ", size:" << item.size();
            } else {
                qCritical() << daemonClient << " related boxClient not found";
            }
        }
    }
}

// 与dbus-daemon 断开连接
void DbusProxy::onDisconnectedServer()
{
    QLocalSocket *sender = static_cast<QLocalSocket *>(QObject::sender());
    if (sender) {
        sender->disconnectFromServer();
    }

    QLocalSocket *boxClient = nullptr;
    for (const auto &client : relations.keys()) {
        if (relations[client] == sender) {
            boxClient = client;
            break;
        }
    }
    if (boxClient) {
        boxClient->disconnectFromServer();
    } else {
        qCritical() << "onDisconnectedServer " << sender << " related boxClient not found";
    }

    // 更新代理与dbus daemon连接关系
    if (connStatus.contains(sender)) {
        connStatus.remove(sender);
    }
    qDebug() << "onDisconnectedServer called sender:" << sender;
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