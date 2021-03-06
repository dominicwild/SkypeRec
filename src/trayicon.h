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

#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>
#include <QPointer>
#include <QMap>
#include <QTime>

#include "common.h"

class QAction;
class QMenu;
class QSignalMapper;
class MainWindow;
class QTimer;

class TrayIcon : public QSystemTrayIcon {
	Q_OBJECT
public:
	TrayIcon(QObject *);
	~TrayIcon();

signals:
	void requestQuit();
	void requestAbout();
	void requestOpenPreferences();
	void requestBrowseCalls();

signals:
	void startRecording(int);
	void stopRecordingAndDelete(int);
	void stopRecording(int);

public slots:
	void startedCall(int, const QString &);
	void stoppedCall(int);
	void startedRecording(int);
	void stoppedRecording(int);
	void connected(bool);

private slots:
	void checkTrayPresence();
	void setWindowedMode();
	void createMainWindow();
	void activate();
	void updateToolTip();

private:
	struct CallData {
		QString skypeName;
		QTime startTime;
		bool isRecording;
		QMenu *menu;
		QAction *startAction;
		QAction *stopAction;
		QAction *stopAndDeleteAction;
	};

	typedef QMap<int, CallData> CallMap;

private:
	void updateIcon();

	QMenu *menu;
	QMenu *aboutMenu;
	QAction *separator;
	CallMap callMap;
	QSignalMapper *smStart;
	QSignalMapper *smStop;
	QSignalMapper *smStopAndDelete;
	QPointer<MainWindow> window;
	bool is_connected;
	QTimer* tooltip_updater;

	DISABLE_COPY_AND_ASSIGNMENT(TrayIcon);
};

#endif

