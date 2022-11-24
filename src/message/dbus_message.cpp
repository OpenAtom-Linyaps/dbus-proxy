/*
 * SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.  
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "dbus_message.h"

#include <QDebug>

/*
 * 根据大小端将字节数组转化为整形
 *
 * @param arr: 报文字节数组
 * @param isBigEndian: 是否为大端序
 *
 * @return int: 转化结果
 */
int byteAraryToInt(const QByteArray &arr, bool isBigEndian)
{
    if (arr.size() < 4) {
        return -1;
    }
    int ret = 0;
    if (isBigEndian) {
        ret = (arr.at(0) << 24) & 0xFF000000;
        ret |= (arr.at(1) << 16) & 0x00FF0000;
        ret |= arr.at(2) << 8 & 0x0000FF00;
        ret |= arr.at(3) & 0x000000FF;
    } else {
        ret = arr.at(0) & 0x000000FF;
        ret |= (arr.at(1) << 8) & 0x0000FF00;
        ret |= (arr.at(2) << 16) & 0x00FF0000;
        ret |= (arr.at(3) << 24) & 0xFF000000;
    }
    return ret;
}

/*
 * 根据偏移量获取8字节对齐结果
 *
 * @param offset: 偏移量
 *
 * @return quint32: 计算结果
 */
quint32 alignBy8(quint32 offset)
{
    return (offset + 8 - 1) & ~(8 - 1);
}

/*
 * 根据偏移量获取4字节对齐结果
 *
 * @param offset: 偏移量
 *
 * @return quint32: 计算结果
 */
quint32 alignBy4(quint32 offset)
{
    return (offset + 4 - 1) & ~(4 - 1);
}

/*
 * 从报文中获取指定偏移量的字符数据
 *
 * @param buffer: 报文字节数组
 * @param header: 报文header
 * @param offset: 偏移量开始地址
 * @param endOffset: 偏移量结束地址
 *
 * @return char*: 计算结果
 */
char *getString(const QByteArray &buffer, Header *header, quint32 *offset, quint32 endOffset)
{
    quint8 len;
    char *str;

    *offset = alignBy4(*offset);
    if (*offset + 4 >= endOffset) {
        return nullptr;
    }

    auto val = buffer.mid(*offset, 4);
    len = byteAraryToInt(val, header->bigEndian);
    *offset += 4;

    if ((*offset) + len + 1 > endOffset) {
        return nullptr;
    }

    if (buffer[(*offset) + len] != '\x0') {
        return nullptr;
    }

    auto dataArray = buffer.mid(*offset, len);
    str = dataArray.data();
    *offset += len + 1;

    return str;
}

/*
 * 从报文中获取dbus消息域签名信息
 *
 * @param buffer: 报文字节数组
 * @param offset: 偏移量开始地址
 * @param endOffset: 偏移量结束地址
 *
 * @return char*: 签名结果
 */
char *getSignature(const QByteArray &buffer, quint32 *offset, quint32 endOffset)
{
    quint8 len;
    char *str;

    if (*offset >= endOffset) {
        return nullptr;
    }

    len = buffer[*offset];
    (*offset)++;

    if ((*offset) + len + 1 > endOffset) {
        return nullptr;
    }

    if (buffer[(*offset) + len] != '\x0') {
        return nullptr;
    }

    auto val = buffer.mid(*offset, len);
    str = val.data();
    *offset += len + 1;

    return str;
}

/*
 * 从报文中解析dbus消息报文头
 *
 * @param buffer: 报文字节数组
 * @param header: 偏移量开始地址
 *
 * @return bool: true:解析成功，false:失败
 */
bool parseHeader(const QByteArray &buffer, Header *header)
{
    // fix 可以使用libdbus api替换
    quint32 arrayLen = 0;
    quint32 headerLen = 0;
    quint32 offset = 0;
    quint32 endOffset = 0;
    quint8 headerType = 0;

    if (buffer.size() < 16) {
        return false;
    }

    // Major protocol version of the sending application.
    if (buffer[3] != '\x1') {
        return false;
    }

    if (buffer[0] == 'B') {
        header->bigEndian = true;
    } else if (buffer[0] == 'l') {
        header->bigEndian = false;
    } else {
        return false;
    }

    // Message type 1 2 3 4 分别表示METHOD_CALL METHOD_RETURN ERROR SIGNAL
    header->type = buffer[1];
    // NO_REPLY_EXPECTED  NO_AUTO_START currently receive 0
    header->flags = buffer[2];
    // Length in bytes of the message body
    auto lengthArray = buffer.mid(4, 4);
    header->length = byteAraryToInt(lengthArray, header->bigEndian);
    // The serial of this message, used as a cookie by the sender to identify the reply corresponding to this request.
    auto serialArray = buffer.mid(8, 4);
    header->serial = byteAraryToInt(serialArray, header->bigEndian);
    qDebug() << QString("parse_header msg header type:%1,flags:%2,length:%3,serial:%4")
                    .arg(header->type)
                    .arg(header->flags)
                    .arg(header->length)
                    .arg(header->serial);
    if (header->serial == 0) {
        return false;
    }

    auto arrLen = buffer.mid(12, 4);
    arrayLen = byteAraryToInt(arrLen, header->bigEndian);

    headerLen = alignBy8(12 + 4 + arrayLen);
    if (headerLen > (quint32)buffer.size()) {
        return false;
    }

    offset = 12 + 4;
    endOffset = offset + arrayLen;

    while (offset < endOffset) {
        // Structs must be 8 byte aligned
        offset = alignBy8(offset);
        if (offset >= endOffset) {
            return false;
        }

        headerType = buffer[offset++];
        if (offset >= endOffset) {
            return false;
        }

        const char *signature = getSignature(buffer, &offset, endOffset);
        if (signature == NULL) {
            return false;
        }
        switch (headerType) {
        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_INVALID: {
            return false;
        }
        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_PATH: {
            if (strcmp(signature, "o") != 0) {
                return false;
            }
            header->path = getString(buffer, header, &offset, endOffset);
            if (header->path == NULL || header->path.isEmpty()) {
                return false;
            }
            break;
        }
        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_INTERFACE: {
            if (strcmp(signature, "s") != 0) {
                return false;
            }
            header->interface = getString(buffer, header, &offset, endOffset);
            if (header->interface == NULL || header->interface.isEmpty()) {
                return false;
            }
            break;
        }
        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_MEMBER: {
            if (strcmp(signature, "s") != 0) {
                return false;
            }
            header->member = getString(buffer, header, &offset, endOffset);
            if (header->member == NULL || header->member.isEmpty()) {
                return false;
            }
            break;
        }
        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_ERROR_NAME: {
            if (strcmp(signature, "s") != 0) {
                return false;
            }
            header->errorName = getString(buffer, header, &offset, endOffset);
            if (header->errorName == NULL || header->errorName.isEmpty()) {
                return false;
            }
            break;
        }
        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_REPLY_SERIAL: {
            if (offset + 4 > endOffset) {
                return false;
            }

            header->hasReplySerial = true;
            // replySerialPos = offset;
            auto val = buffer.mid(offset, 4);
            header->replySerial = byteAraryToInt(val, header->bigEndian);
            offset += 4;
            break;
        }
        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_DESTINATION: {
            if (strcmp(signature, "s") != 0) {
                return false;
            }
            header->destination = getString(buffer, header, &offset, endOffset);
            if (header->destination == NULL || header->destination.isEmpty()) {
                return false;
            }
            break;
        }

        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_SENDER: {
            if (strcmp(signature, "s") != 0) {
                return false;
            }
            header->sender = getString(buffer, header, &offset, endOffset);
            if (header->sender == NULL || header->sender.isEmpty()) {
                return false;
            }
            break;
        }

        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_SIGNATURE: {
            if (strcmp(signature, "g") != 0) {
                return false;
            }
            header->signature = getSignature(buffer, &offset, endOffset);
            if (header->signature == NULL || header->signature.isEmpty()) {
                return false;
            }
            break;
        }

        case (int)DBusMessageHeaderField::DBUS_MESSAGE_HEADER_FIELD_NUM_UNIX_FDS: {
            if (offset + 4 > endOffset) {
                return false;
            }
            auto fdsVal = buffer.mid(offset, 4);
            header->unixFds = byteAraryToInt(fdsVal, header->bigEndian);
            offset += 4;
            break;
        }
        default:
            return false;
        }
    }
    qDebug()
        << QString(
               "parseHeader msg serial:%1, reply_serial:%2, hasReplySerial:%3, destination:%4, path:%5, interface:%6, member:%7")
               .arg(header->serial)
               .arg(header->replySerial)
               .arg(header->hasReplySerial)
               .arg(header->destination)
               .arg(header->path)
               .arg(header->interface)
               .arg(header->member);

    switch (header->type) {
    case (int)MessageType::METHOD_CALL:
        if (header->path == NULL || header->member == NULL) {
            return false;
        }
        break;

    case (int)MessageType::METHOD_RETURN:
        if (!header->hasReplySerial) {
            return false;
        }
        break;

    case (int)MessageType::ERROR:
        if (header->errorName == NULL || !header->hasReplySerial) {
            return false;
        }
        break;

    case (int)MessageType::SIGNAL:
        if (header->path == NULL || header->interface == NULL || header->member == NULL) {
            return false;
        }
        if (header->path == "/org/freedesktop/DBus/Local" || header->interface == "org.freedesktop.DBus.Local") {
            return false;
        }
        break;

    default:
        return false;
    }
    return true;
}

/*
 * 从报文中解析dbus消息报文头
 *
 * @param byteArray: 报文字节数组
 * @param header: dbus消息报文头
 *
 * @return bool: true:解析成功 false:失败
 */
bool parseDBusMsg(const QByteArray &byteArray, Header *header)
{
    DBusError dbErr;
    dbus_error_init(&dbErr);

    // invalid type cannot be marshaled
    // msg: "l\x00\x00\x1C\x00\x00\x00org.fcitx.Fcitx.InputContext\x00" , msg size: 36
    if (byteArray[1] == 0) {
        return false;
    }
    DBusMessage *receiveMsg = dbus_message_demarshal(byteArray.constData(), byteArray.size(), &dbErr);
    if (!receiveMsg) {
        qCritical() << "dbus_message_demarshal failed";
        if (dbus_error_is_set(&dbErr)) {
            qCritical() << "dbus_message_demarshal err info:" << dbErr.message << ", dbus msg:" << byteArray
                        << ", size:" << byteArray.size();
            dbus_error_free(&dbErr);
        }
        return false;
    } else {
        header->destination = QString(QLatin1String(dbus_message_get_destination(receiveMsg)));
        header->path = QString(QLatin1String(dbus_message_get_path(receiveMsg)));
        header->interface = QString(QLatin1String(dbus_message_get_interface(receiveMsg)));
        header->member = QString(QLatin1String(dbus_message_get_member(receiveMsg)));
        header->serial = dbus_message_get_serial(receiveMsg);
        header->replySerial = dbus_message_get_reply_serial(receiveMsg);
        header->hasReplySerial = header->replySerial > 0 ? true : false;
        header->sender = QString(QLatin1String(dbus_message_get_sender(receiveMsg)));
        header->type = dbus_message_get_type(receiveMsg);
        header->flags = byteArray[2];
        dbus_message_unref(receiveMsg);
    }
    return true;
}

/*
 * 将报文数组分隔成符合dbus协议标准的dbus消息
 *
 * @param buffer: 报文字节数组
 * @param out: dbus消息List
 */
void splitDBusMsg(const QByteArray &buffer, QList<QByteArray> &out)
{
    if (buffer.startsWith("BEGIN")) {
        out.push_back(buffer.left(7));
        out.push_back(buffer.mid(7));
        return;
    }

    // 握手消息
    if (buffer[0] != 'B' && buffer[0] != 'l') {
        out.push_back(buffer);
        return;
    }

    if (buffer.size() < 16) {
        out.push_back(buffer);
        return;
    }

    bool bigEndian;
    if (buffer[0] == 'B') {
        bigEndian = true;
    } else {
        bigEndian = false;
    }

    QByteArray tmp = buffer;
    while (true) {
        // Length in bytes of the message body
        auto bodyLenArray = tmp.mid(4, 4);
        auto bodyLen = byteAraryToInt(bodyLenArray, bigEndian);
        // A UINT32 giving the length of the array data in bytes
        auto arrLen = tmp.mid(12, 4);
        int arrayLen = byteAraryToInt(arrLen, bigEndian);
        int headerLen = alignBy8(12 + 4 + arrayLen);
        out.push_back(tmp.left(bodyLen + headerLen));
        if (bodyLen + headerLen >= tmp.size()) {
            break;
        }
        tmp = tmp.mid(bodyLen + headerLen);
    }
}