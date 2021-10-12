/*
 * Copyright (C) 2021 Uniontech Technology Co., Ltd.
 *
 * Author:     yeshanshan <yeshanshan@uniontech.com>
 *
 * Maintainer: yeshanshan <yeshanshan@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QFile>
#include <QQueue>
#include <QString>
#include <functional>
#include <QRegularExpression>
#include <DStandardPaths>

using ConnKey = QString;
using ResourceKey = QString;
using ConnServiceName = QString;
using ConnRefCount = int;

inline QString getResourceKey(const ConnKey &connKey)
{
    return connKey.left(connKey.lastIndexOf('/'));
}
inline uint getConnectionKey(const ConnKey &connKey)
{
    return connKey.mid(connKey.lastIndexOf('/') + 1).toUInt();
}

struct ConfigureId {
    QString appid;
    QString subpath;
    QString resource;
};

inline ConfigureId getAppConfigureId(const QString &path)
{
    ConfigureId info;
    // /usr/share/dsg/apps/$appid/configs/[$subpath]/$resource.json
    static QRegularExpression usrReg(R"(^/usr/share/dsg/apps/(?<appid>[a-z0-9\s\-_\@\-\^!#$%&]+)/configs(?<subpath>(/[a-z0-9\s\-_\@\-\^!#$%&])*)/(?<resource>[a-z0-9\s\-_\@\-\^!#$%&]+).json$)");
    // /opt/apps/$appid/files/schemas/configs/[$subpath]/$resource.json . e.g:
    static QRegularExpression optReg(R"(^/opt/apps/(?<appid>[a-z0-9\s\-_\@\-\^!#$%&]+)/files/schemas/configs(?<subpath>(/[a-z0-9\s\-_\@\-\^!#$%&])*)/(?<resource>[a-z0-9\s\-_\@\-\^!#$%&]+).json$)");

    QRegularExpressionMatch match;
    match = usrReg.match(path);
    if (!match.hasMatch()) {
        match = optReg.match(path);
        if (!match.hasMatch()) {
            return info;
        }
    }
    info.appid = match.captured("appid");
    info.subpath = match.captured("subpath");
    info.resource = match.captured("resource");

    return info;
}

inline ConfigureId getGenericConfigureId(const QString &path)
{
    ConfigureId info;
    DCORE_USE_NAMESPACE;

    DStandardPaths::filePath(DStandardPaths::DSG::DataDir, QString("configs"));
    // /usr/share/dsg/apps/$appid/configs/[$subpath]/$resource.json
    static QRegularExpression genericReg(QString(R"(^%1(?<subpath>(/[a-z0-9\s\-_\@\-\^!#$%&])*)/(?<resource>[a-z0-9\s\-_\@\-\^!#$%&]+).json$)").arg(
                DStandardPaths::filePath(DStandardPaths::DSG::DataDir, QString("configs"))));

    QRegularExpressionMatch match = genericReg.match(path);
    if (match.hasMatch()) {
        info.subpath = match.captured("appid");
        info.resource = match.captured("subpath");
    }

    return info;
}


template<class T>
class ObjectPool
{
public:
    typedef T* DataType;
    ~ObjectPool()
    {
        qDeleteAll(m_pool);
        m_pool.clear();
    }
    using InitFunc = std::function<void(DataType)>;
    void setInitFunc(InitFunc func) { m_initFunc = func;}

    DataType pull()
    {
        if (m_pool.isEmpty()) {
            auto item = new T();
            if (m_initFunc)
                m_initFunc(item);

            return item;
        }
        return m_pool.dequeue();
    }
    void push(DataType item) { m_pool.enqueue(item);}

private:
    QQueue<DataType> m_pool;
    InitFunc m_initFunc;
};

inline QString getProcessNameByPid(const uint pid)
{
#ifdef Q_OS_LINUX
    const QString desc = QString("/proc/%1/status").arg(pid);
    QFile file(desc);
    if(file.open(QIODevice::ReadOnly)) {
        const QString &name = file.readLine();
        return name.mid(name.indexOf(':') + 1).trimmed();
    }
#endif // Q_OS_LINUX
    return QString::number(pid);
}

#ifdef Q_OS_LINUX
#include <pwd.h>
#endif // Q_OS_LINUX
inline QString getUserNameByUid(const uint uid)
{
#ifdef Q_OS_LINUX
    passwd *passwd = getpwuid(uid);
    return QString::fromLocal8Bit(passwd->pw_name);
#else // Q_OS_LINUX
    return QString::number(uid);
#endif
}