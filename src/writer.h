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

#ifndef WRITER_H
#define WRITER_H

#include <QFile>
#include <QDateTime>
#include <QString>

#include "common.h"

class QByteArray;

class AudioFileWriter {
public:
	AudioFileWriter();
	virtual ~AudioFileWriter();

	// tags should be set before open() if possible, but can also be set
	// and re-set later, but not after close().  tags will be written to
	// disk at arbitrary times, but close() guarantees they are written.
	// however, if a writer doesn't support tags at all, they are silently
	// ignored.
	virtual void setTags(const QString &, const QDateTime &);

	// Note: you're not supposed to reopen after a close
	virtual bool open(const QString &, long, bool);
	virtual void close();
	virtual bool write(const QByteArray &, const QByteArray &, long, bool = false) = 0;
	QString fileName() const { return file.fileName(); }

protected:
	QFile file;
	long sampleRate;
	bool stereo;
	qint64 samplesWritten;
	QString tagComment;
	QDateTime tagTime;
	bool mustWriteTags;

	DISABLE_COPY_AND_ASSIGNMENT(AudioFileWriter);
};

#endif

