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

#include "filter/dbus_filter.h"

TEST(filter, filter01)
{
    // com.deepin.linglong.AppManager /com/deepin/linglong/PackageManager com.deepin.linglong.PackageManager.test
    DbusFilter filter;
    filter.addNameFilter("com.deepin.linglong.*");
    filter.addPathFilter("/com/deepin/linglong/*");
    filter.addInterfaceFilter("com.deepin.linglong.PackageManager.*");

    QString name1 = "com.deepin.linglong.AppManager";
    QString path1 = "/com/deepin/linglong/PackageManager";
    QString interface1 = "com.deepin.linglong.PackageManager.test";
    bool ret = filter.isMessageMatch(name1, path1, "");
    EXPECT_EQ(ret, true);
    QString name2 = "com.deepin.test.AppManager";
    QString path2 = "/com/deepin/test";
    QString interface2 = "com.deepin.linglong.test";
    ret = filter.isMessageMatch(name2, path2, "");
    EXPECT_EQ(ret, false);
}

TEST(filter, filter02)
{
    // com.deepin.linglong.AppManager /com/deepin/linglong/PackageManager com.deepin.linglong.PackageManager.test
    DbusFilter filter;
    filter.addNameFilter("com.deepin.linglong.AppManager");
    filter.addPathFilter("/com/deepin/linglong/PackageManager");
    filter.addInterfaceFilter("com.deepin.linglong.PackageManager");

    QString name1 = "com.deepin.linglong.AppManager";
    QString path1 = "/com/deepin/linglong/PackageManager";
    QString interface1 = "com.deepin.linglong.PackageManager";
    bool ret = filter.isMessageMatch(name1, path1, "");
    EXPECT_EQ(ret, true);
    QString name2 = "com.deepin.test.AppManager";
    QString path2 = "/com/deepin/test";
    QString interface2 = "com.deepin.linglong.test";
    ret = filter.isMessageMatch(name2, path2, "");
    EXPECT_EQ(ret, false);
}

TEST(filter, dump01)
{
    // com.deepin.linglong.AppManager /com/deepin/linglong/PackageManager com.deepin.linglong.PackageManager.test
    DbusFilter filter;
    filter.addNameFilter("com.deepin.linglong.*");
    filter.addPathFilter("/com/deepin/linglong/*");
    filter.addInterfaceFilter("com.deepin.linglong.PackageManager.*");

    filter.addNameFilter("org.freedesktop.portal");
    filter.addPathFilter("/org/freedesktop/portal/");
    filter.addInterfaceFilter("org.freedesktop.portal.document");

    QString config = "";
    filter.dumpConfig(config);
    EXPECT_EQ(config.isEmpty(), false);
}