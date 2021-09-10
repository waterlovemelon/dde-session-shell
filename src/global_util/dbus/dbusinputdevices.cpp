
#include <sys/time.h>
#define TRACE_ME_IN struct timeval tp ; gettimeofday ( &tp , nullptr ); printf("[%4ld.%4ld] In: %s\n",tp.tv_sec , tp.tv_usec,__PRETTY_FUNCTION__);
#define TRACE_ME_OUT gettimeofday (const_cast<timeval *>(&tp) , nullptr ); printf("[%4ld.%4ld] Out: %s\n",tp.tv_sec , tp.tv_usec,__PRETTY_FUNCTION__);

/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -p dbusinputdevices -c DBusInputDevices com.deepin.daemon.InputDevices.xml
 *
 * qdbusxml2cpp is Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#include "dbusinputdevices.h"

/*
 * Implementation of interface class DBusInputDevices
 */

QDBusArgument &operator<<(QDBusArgument &argument, const InputDevice &device)
{
    TRACE_ME_IN;	//<<==--TracePoint!
    argument.beginStructure();
    argument << device.interface << device.deviceType;
    argument.endStructure();
    TRACE_ME_OUT;	//<<==--TracePoint!
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, InputDevice &device)
{
    TRACE_ME_IN;	//<<==--TracePoint!
    argument.beginStructure();
    argument >> device.interface >> device.deviceType;
    argument.endStructure();
    TRACE_ME_OUT;	//<<==--TracePoint!
    return argument;
}

DBusInputDevices::DBusInputDevices(QObject *parent)
    : QDBusAbstractInterface(staticServiceName(), staticObjectPath(), staticInterfaceName(), QDBusConnection::sessionBus(), parent)
{
    TRACE_ME_IN;	//<<==--TracePoint!
    qDBusRegisterMetaType<InputDevice>();
    qDBusRegisterMetaType<InputDeviceList>();

    QDBusConnection::sessionBus().connect(this->service(), this->path(), "org.freedesktop.DBus.Properties",  "PropertiesChanged","sa{sv}as", this, SLOT(__propertyChanged__(QDBusMessage)));
    TRACE_ME_OUT;	//<<==--TracePoint!

}

DBusInputDevices::~DBusInputDevices()
{
    TRACE_ME_IN;	//<<==--TracePoint!
    QDBusConnection::sessionBus().disconnect(service(), path(), "org.freedesktop.DBus.Properties",  "PropertiesChanged",  "sa{sv}as", this, SLOT(propertyChanged(QDBusMessage)));
    TRACE_ME_OUT;	//<<==--TracePoint!

}

