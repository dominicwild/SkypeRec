/*
	SkypeRec
	Copyright 2008-2009 by jlh <jlh@gmx.ch>
	Copyright 2010-2011 by Peter Savichev  (proton) <psavichev@gmail.com>

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation; either version 2 of the License, version 3 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

	The GNU General Public License version 2 is included with the source of
	this program under the file name COPYING.  You can also get a copy on
	http://www.fsf.org/
*/

#include <QList>
#include <QVariant>
#include <QTimer>
#include <QMessageBox>
#include <QtDBus>

#include "skype.h"
#include "common.h"

namespace
{
	const QString skypeServiceName("com.Skype.API");
	const QString skypeInterfaceName("com.Skype.API");
}

Skype::Skype(QObject *parent) : QObject(parent), dbus("SkypeRecorder"), connectionState(CONNECTION_STATE::DISONNECTED)
{
	timer = new QTimer(this);
	timer->setInterval(5000);
	connect(timer, SIGNAL(timeout()), this, SLOT(poll()));

	dbus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "SkypeRecorder");

	if (!dbus.isConnected())
	{
		debug("Error: Cannot connect to DBus");
		QMessageBox::critical(NULL, PROGRAM_NAME " - Error",
			QString("The connection to DBus failed!  This is a fatal error."));
		return;
	}

	// export our object
	exported = new SkypeExport(this);
	if (!dbus.registerObject("/com/Skype/Client", this))
	{
		debug("Error: Cannot register object /com/Skype/Client");
		QMessageBox::critical(NULL, PROGRAM_NAME " - Error",
			QString("Cannot register object on DBus!  This is a fatal error."));
		return;
	}

	connect(dbus.interface(), SIGNAL(serviceOwnerChanged(const QString &, const QString &, const QString &)),
		this, SLOT(serviceOwnerChanged(const QString &, const QString &, const QString &)));
	QTimer::singleShot(0, this, SLOT(connectToSkype()));
}

void Skype::connectToSkype()
{
	if(connectionState) return;

	QDBusReply<bool> exists = dbus.interface()->isServiceRegistered(skypeServiceName);

	if (!exists.isValid() || !exists.value())
	{
		timer->stop();
		debug(QString("Service %1 not found on DBus").arg(skypeServiceName));
		return;
	}

	if (!timer->isActive()) timer->start();

	sendWithAsyncReply("NAME SkypeCallRecorder");
	connectionState = CONNECTION_STATE::CONNECTED;
}

void Skype::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
	if (name != skypeServiceName) return;

	if (oldOwner.isEmpty())
	{
		debug(QString("DBUS: Skype API service appeared as %1").arg(newOwner));
		if (connectionState != CONNECTION_STATE::CONNECTED3)
			connectToSkype();
	}
	else if (newOwner.isEmpty())
	{
		debug("DBUS: Skype API service disappeared");
		if (connectionState == CONNECTION_STATE::CONNECTED3)
			emit connected(false);
		timer->stop();
		connectionState = CONNECTION_STATE::DISONNECTED;
	}
}

void Skype::sendWithAsyncReply(const QString &s)
{
	debug(QString("SKYPE --> %1 (async reply)").arg(s));

	QDBusMessage msg = QDBusMessage::createMethodCall(skypeServiceName, "/com/Skype", skypeInterfaceName, "Invoke");
	QList<QVariant> args;
	args.append(s);
	msg.setArguments(args);

	dbus.callWithCallback(msg, this, SLOT(methodCallback(const QDBusMessage &)), SLOT(methodError(const QDBusError &, const QDBusMessage &)), 3600000);
}

QString Skype::sendWithReply(const QString &s, int timeout)
{
	debug(QString("SKYPE --> %1 (sync reply)").arg(s));

	QDBusMessage msg = QDBusMessage::createMethodCall(skypeServiceName, "/com/Skype", skypeInterfaceName, "Invoke");
	QList<QVariant> args;
	args.append(s);
	msg.setArguments(args);

	msg = dbus.call(msg, QDBus::Block, timeout);

	if (msg.type() != QDBusMessage::ReplyMessage)
	{
		debug(QString("SKYPE <R- (failed)"));
		return QString();
	}

	QString ret = msg.arguments().value(0).toString();
	debug(QString("SKYPE <R- %1").arg(ret));
	return ret;
}

void Skype::send(const QString &s)
{
	debug(QString("SKYPE --> %1 (no reply)").arg(s));

	QDBusMessage msg = QDBusMessage::createMethodCall(skypeServiceName, "/com/Skype", skypeInterfaceName, "Invoke");
	QList<QVariant> args;
	args.append(s);
	msg.setArguments(args);

	dbus.call(msg, QDBus::NoBlock);
}

QString Skype::getObject(const QString &object)
{
	QString ret = sendWithReply("GET " + object);
	if (!ret.startsWith(object))
		return QString();
	return ret.mid(object.size() + 1);
}

void Skype::methodCallback(const QDBusMessage &msg)
{
	if (msg.type() != QDBusMessage::ReplyMessage)
	{
		connectionState = CONNECTION_STATE::DISONNECTED;
		emit connectionFailed("Cannot communicate with Skype");
		return;
	}

	QString s = msg.arguments().value(0).toString();
	debug(QString("SKYPE <R- %1").arg(s));

	if (connectionState == CONNECTION_STATE::CONNECTED)
	{
		if (s == "OK")
		{
			connectionState = CONNECTION_STATE::CONNECTED2;
			sendWithAsyncReply("PROTOCOL 5");
		}
		else if (s == "CONNSTATUS OFFLINE")
		{
			// no user logged in, cannot connect now.  from now on,
			// we have no way of knowing when the user has logged
			// in and we may again try to connect.  this is an
			// annoying limitation of the Skype API which we work
			// around be polling
			connectionState = CONNECTION_STATE::DISONNECTED;
		}
		else
		{
			connectionState = CONNECTION_STATE::DISONNECTED;
			emit connectionFailed("Skype denied access");
		}
	}
	else if (connectionState == CONNECTION_STATE::CONNECTED2)
	{
		if (s == "PROTOCOL 5")
		{
			connectionState = CONNECTION_STATE::CONNECTED3;
			emit connected(true);
		}
		else
		{
			connectionState = CONNECTION_STATE::DISONNECTED;
			emit connectionFailed("Skype handshake error");
		}
	}
}

void Skype::methodError(const QDBusError &error, const QDBusMessage &)
{
	connectionState = CONNECTION_STATE::DISONNECTED;
	emit connectionFailed(error.message());
}

void Skype::doNotify(const QString &s)
{
	if (connectionState != CONNECTION_STATE::CONNECTED3) return;

	debug(QString("SKYPE <-- %1").arg(s));

	if (s.startsWith("CURRENTUSERHANDLE "))
		skypeName = s.mid(18);

	emit notify(s);
}

void Skype::poll()
{
	if (connectionState == CONNECTION_STATE::DISONNECTED)
	{
		connectToSkype();
	}
	else if (connectionState == CONNECTION_STATE::CONNECTED3)
	{
		if (sendWithReply("PING", 2000) != "PONG")
		{
			debug("Skype didn't reply with PONG to our PING");
			connectionState = CONNECTION_STATE::DISONNECTED;
			emit connected(false);
		}
	}
}

// ---- SkypeExport ----

SkypeExport::SkypeExport(Skype *p) : QDBusAbstractAdaptor(p), parent(p)
{
}

void SkypeExport::Notify(const QString &s)
{
	parent->doNotify(s);
}

