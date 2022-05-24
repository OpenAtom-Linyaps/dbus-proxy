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

#include "message/dbus_message.h"

TEST(dbusmsg, message01)
{
    QByteArray byteArray(
        "l\x03\x01\x01\x42\x00\x00\x00\x10\x00\x00\x00g\x00\x00\x00\x04\x01s\x00(\x00\x00\x00org.freedesktop.DBus.Error.UnknownMethod\x00\x00\x00\x00\x00\x00\x00\x00\x06\x01s\x00\x06\x00\x00\x00:1.585\x00\x00\x05\x01u\x00\x02\x00\x00\x00\b\x01g\x00\x01s\x00\x00\x07\x01s\x00\x06\x00\x00\x00:1.298\x00\x00\x3D\x00\x00\x00org.freedesktop.DBus.Error.AccessDenied, dbus msg hijack test\x00",
        186);
    Header header;
    bool ret = parseHeader(byteArray, &header);
    qInfo() << "msg path:" << header.path << ", interface:" << header.interface << ",member:" << header.member
            << ", dest:" << header.destination;
    EXPECT_EQ(ret, true);
    bool isErrNameOk = (header.errorName == "org.freedesktop.DBus.Error.UnknownMethod");
    EXPECT_EQ(isErrNameOk, true);
    bool isDstOk = (header.destination == ":1.585");
    EXPECT_EQ(isDstOk, true);
    bool isSenderOk = (header.sender == ":1.298");
    EXPECT_EQ(isSenderOk, true);
}

TEST(dbusmsg, message02)
{
    QByteArray byteArray(
        "l\x01\x00\x01\x14\x00\x00\x00\x02\x00\x00\x00\x9F\x00\x00\x00\x01\x01o\x00#\x00\x00\x00/com/deepin/linglong/PackageManager\x00\x00\x00\x00\x00\x02\x01s\x00\"\x00\x00\x00"
        "com.deepin.linglong.PackageManager\x00\x00\x00\x00\x00\x00\x03\x01s\x00\x04\x00\x00\x00test\x00\x00\x00\x00\x06\x01s\x00\x1E\x00\x00\x00"
        "com.deepin.linglong.AppManager\x00\x00\b\x01g\x00\x01s\x00\x00\x0F\x00\x00\x00org.deepin.demo\x00",
        196);
    Header header;
    bool ret = parseHeader(byteArray, &header);
    qInfo() << "msg path:" << header.path << ", interface:" << header.interface << ",member:" << header.member
            << ", dest:" << header.destination;
    EXPECT_EQ(ret, true);
    bool isPathOk = (header.path == "/com/deepin/linglong/PackageManager");
    EXPECT_EQ(isPathOk, true);
    bool isInterfaceOk = (header.interface == "com.deepin.linglong.PackageManager");
    EXPECT_EQ(isInterfaceOk, true);
    bool isMemberOk = (header.member == "test");
    EXPECT_EQ(isMemberOk, true);
}

TEST(dbusmsg, message03)
{
    // 正常调用 回复的消息返回值为String
    // "l\x02\x01\x01\x14\x00\x00\x00\x06\x00\x00\x00""0\x00\x00\x00\x06\x01s\x00\x07\x00\x00\x00:1.5475\x00\x05\x01u\x00\x02\x00\x00\x00\b\x01g\x00\x01s\x00\x00\x07\x01s\x00\x07\x00\x00\x00:1.5447\x00\x0F\x00\x00\x00org.deepin.demo\x00"
    QByteArray byteArray(
        "l\x02\x01\x01\x14\x00\x00\x00\x06\x00\x00\x00"
        "0\x00\x00\x00\x06\x01s\x00\x07\x00\x00\x00:1.5475\x00\x05\x01u\x00\x02\x00\x00\x00\b\x01g\x00\x01s\x00\x00\x07\x01s\x00\x07\x00\x00\x00:1.5447\x00\x0F\x00\x00\x00org.deepin.demo\x00",
        84);
    Header header;
    bool ret = parseHeader(byteArray, &header);
    EXPECT_EQ(ret, true);
    bool isDstOk = (header.destination == ":1.5475");
    EXPECT_EQ(isDstOk, true);
    bool isSenderOk = (header.sender == ":1.5447");
    EXPECT_EQ(isSenderOk, true);
    qInfo() << "msg sender:" << header.sender << ", dest:" << header.destination;
}

// buffer[0] 大小端  buffer[1] Message type 1 2 3 4 分别表示METHOD_CALL METHOD_RETURN ERROR SIGNAL
// buffer[2] 为flags buffer[3] 为版本号 buffer[4] ;开始的4字节为消息body的长度，值为59 buffer[8] 开始的4字节 为serial号
// buffer[12] 为数组长度  \x06 表示HeadType 代表DESTINATION x01s\x00 代表签名 \x07开始的4字节 类型长度  \x04 为errname
// (\x00\x00\x00 为长度40  \x05 为 REPLY_SERIAL为2 \b 值为8类型为SIGNATURE  \x07 代表发送方 \x14\x00\x00\x00
// 类型长度为20     org开头的为消息正文     he body is made up of arguments. "l\x03\x01\x01 ;\x00\x00\x00
// \x03\x00\x00\x00  u\x00\x00\x00 \x06\x01s\x00  \x07\x00\x00\x00  :1.6957\x00 \x04\x01s\x00 (\x00\x00\x00 org. free
// desk top. DBus .Err or.U nkno wnMe thod \x00\x00\x00\x00 \x00\x00\x00\x00 \x05\x01u\x00 \x02\x00\x00\x00  \b\x01g\x00
// \x01s\x00\x00 \x07\x01s\x00 \x14\x00\x00\x00 org. free desk top. DBus \x00\x00\x00\x00 ""6\x00\x00\x00
// org.freedesktop.DBus does not understand message Ping1\x00"
TEST(dbusmsg, message04)
{
    QByteArray byteArray(
        "l\x01\x00\x01\x00\x00\x00\x00\x01\x00\x00\x00n\x00\x00\x00\x01\x01o\x00\x15\x00\x00\x00/org/freedesktop/DBus\x00\x00\x00\x06\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x02\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x03\x01s\x00\x05\x00\x00\x00Hello\x00\x00\x00",
        128);
    qInfo() << byteArray;
    Header header;
    bool ret = parseHeader(byteArray, &header);
    qInfo() << "msg path:" << header.path << ", interface:" << header.interface << ",member:" << header.member
            << ", dest:" << header.destination;
    EXPECT_EQ(ret, true);
    bool isMemberOk = (header.member == "Hello");
    EXPECT_EQ(isMemberOk, true);
}

TEST(dbusmsg, message05)
{
    QByteArray byteArray(
        "l\x01\x00\x01\xA2\x00\x00\x00\x07\x00\x00\x00y\x00\x00\x00\x01\x01o\x00\x15\x00\x00\x00/org/freedesktop/DBus\x00\x00\x00\x02\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x06\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\b\x01g\x00\x01s\x00\x00\x03\x01s\x00\b\x00\x00\x00""AddMatch\x00\x00\x00\x00\x00\x00\x00\x00\x9D\x00\x00\x00type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop.DBus',member='NameOwnerChanged',path='/org/freedesktop/DBus',arg0='org.gtk.vfs.Daemon'\x00l\x01\x00\x01\x1C\x00\x00\x00\b\x00\x00\x00\x83\x00\x00\x00\x01\x01o\x00\x15\x00\x00\x00/org/freedesktop/DBus\x00\x00\x00\x02\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x06\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\b\x01g\x00\x02su\x00\x03\x01s\x00\x12\x00\x00\x00StartServiceByName\x00\x00\x00\x00\x00\x00\x12\x00\x00\x00org.gtk.vfs.Daemon\x00\x00\x00\x00\x00\x00",
        486);
    qInfo() << byteArray;
    QList<QByteArray> out;
    splitDBusMsg(byteArray, out);
    EXPECT_EQ(out.size(), 2);
}

TEST(dbusmsg, message06)
{
    QByteArray byteArray(
        "BEGIN\r\nl\x01\x00\x01\x00\x00\x00\x00\x01\x00\x00\x00n\x00\x00\x00\x01\x01o\x00\x15\x00\x00\x00/org/freedesktop/DBus\x00\x00\x00\x06\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x02\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x03\x01s\x00\x05\x00\x00\x00Hello\x00\x00\x00",
        135);
    qInfo() << byteArray;
    QList<QByteArray> out;
    splitDBusMsg(byteArray, out);
    EXPECT_EQ(out.size(), 2);
}

TEST(dbusmsg, message07)
{
    QByteArray byteArray(
        "l\x01\x00\x01\x00\x00\x00\x00\x01\x00\x00\x00n\x00\x00\x00\x01\x01o\x00\x15\x00\x00\x00/org/freedesktop/DBus\x00\x00\x00\x06\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x02\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x03\x01s\x00\x05\x00\x00\x00Hello\x00\x00\x00",
        128);
    qInfo() << byteArray;
    Header header;
    bool ret = parseDBusMsg(byteArray, &header);
    qInfo() << "msg path:" << header.path << ", interface:" << header.interface << ",member:" << header.member
            << ", dest:" << header.destination;
    EXPECT_EQ(ret, true);
    bool isMemberOk = (header.member == "Hello");
    EXPECT_EQ(isMemberOk, true);
}