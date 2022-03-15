/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dbus_filter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegExp>

/*
 * 判断是否为符合规则的表达式
 *
 * @param expression: 表达式
 *
 * @return bool: true: 是 false:否
 */
bool DbusFilter::isRegularExp(const QString &expression)
{
    if (expression.endsWith("*") || expression.endsWith("+") || expression.endsWith("?")) {
        return true;
    }
    return false;
}

/*
 * 判断输入字符串是否匹配指定规则
 *
 * @param src: 输入表达式
 * @param reg: 正则表达式
 *
 * @return bool: true: 是 false:否
 */
bool DbusFilter::isMatchRegExp(const QString &src, const QString &reg)
{
    QRegExp regx(reg);
    if (src.contains(regx)) {
        return true;
    }
    return false;
}

/*
 * 判断输入数据是否匹配指定规则列表
 *
 * @param data: 输入表达式
 * @param filterList: 规则列表
 *
 * @return bool: true: 是 false:否
 */
bool DbusFilter::isMatchFilter(const QString &data, const QStringList &filterList)
{
    bool isFound = false;
    for (QString item : filterList) {
        if (item == data || (isRegularExp(item) && isMatchRegExp(data, item))) {
            isFound = true;
            break;
        }
    }
    if (!isFound) {
        return false;
    }
    return true;
}

/*
 * 判断dbus消息是否匹配规则列表
 *
 * @param name: 消息名称
 * @param path: 消息路径
 * @param interface: 消息interface
 *
 * @return bool: true: 是 false:否
 */
bool DbusFilter::isMessageMatch(const QString &name, const QString &path, const QString &interface)
{
    if (name.isEmpty() && path.isEmpty() && interface.isEmpty()) {
        return false;
    }
    if (!name.isEmpty() && !isMatchFilter(name, nameFilter)) {
        return false;
    }
    if (!path.isEmpty() && !isMatchFilter(path, pathFilter)) {
        return false;
    }
    if (!interface.isEmpty() && !isMatchFilter(interface, interfaceFilter)) {
        return false;
    }
    return true;
}

/*
 * 添加消息名称匹配规则
 *
 * @param name: 消息名称匹配规则
 */
void DbusFilter::addNameFilter(const QString &name)
{
    if (nameFilter.contains(name)) {
        return;
    }
    nameFilter.append(name);
}

/*
 * 添加消息路径匹配规则
 *
 * @param path: 消息路径匹配规则
 */
void DbusFilter::addPathFilter(const QString &path)
{
    if (pathFilter.contains(path)) {
        return;
    }
    pathFilter.append(path);
}

/*
 * 添加消息interface匹配规则
 *
 * @param interface: 消息interface匹配规则
 */
void DbusFilter::addInterfaceFilter(const QString &interface)
{
    if (interfaceFilter.contains(interface)) {
        return;
    }
    interfaceFilter.append(interface);
}

/*
 * dump dbus消息过滤规则
 *
 * @param config: 输出结果
 */
void DbusFilter::dumpConfig(QString &config)
{
    QJsonObject item;
    item["name"] = QJsonArray::fromStringList(nameFilter);
    item["path"] = QJsonArray::fromStringList(pathFilter);
    item["interface"] = QJsonArray::fromStringList(interfaceFilter);
    QJsonObject obj;
    obj["dbuspermission"] = item;
    QJsonDocument doc(obj);
    config = doc.toJson();
    qInfo().noquote() << config;
}