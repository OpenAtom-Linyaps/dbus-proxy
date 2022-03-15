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

#include <QDebug>
#include <QObject>
#include <QStringList>

class DbusFilter : public QObject
{
    Q_OBJECT

private:
    // dbus 消息对应的name path interface
    QStringList nameFilter;
    QStringList pathFilter;
    QStringList interfaceFilter;

    /*
     * 判断是否为符合规则的表达式
     *
     * @param expression: 表达式
     *
     * @return bool: true: 是 false:否
     */
    bool isRegularExp(const QString &expression);

    /*
     * 判断输入字符串是否匹配指定规则
     *
     * @param src: 输入表达式
     * @param reg: 正则表达式
     *
     * @return bool: true: 是 false:否
     */
    bool isMatchRegExp(const QString &src, const QString &reg);

    /*
     * 判断输入数据是否匹配指定规则列表
     *
     * @param data: 输入表达式
     * @param filterList: 规则列表
     *
     * @return bool: true: 是 false:否
     */
    bool isMatchFilter(const QString &data, const QStringList &filterList);

public:
    /*
     * 判断dbus消息是否匹配规则列表
     *
     * @param name: 消息名称
     * @param path: 消息路径
     * @param interface: 消息interface
     *
     * @return bool: true: 是 false:否
     */
    bool isMessageMatch(const QString &name, const QString &path, const QString &interface);

    /*
     * 添加消息名称匹配规则
     *
     * @param name: 消息名称匹配规则
     */
    void addNameFilter(const QString &name);

    /*
     * 添加消息路径匹配规则
     *
     * @param path: 消息路径匹配规则
     */
    void addPathFilter(const QString &path);

    /*
     * 添加消息interface匹配规则
     *
     * @param interface: 消息interface匹配规则
     */
    void addInterfaceFilter(const QString &interface);

    /*
     * dump dbus消息过滤规则
     *
     * @param config: 输出结果
     */
    void dumpConfig(QString &config);
};
