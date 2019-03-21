/*
 * Copyright 2019 Clement Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef DFHACK_CLIENT_QT_DFHACK_NOTIFIER
#define DFHACK_CLIENT_QT_DFHACK_NOTIFIER

#include <QObject>
#include <dfhack-client-qt/globals.h>

namespace DFHack
{

/**
 * Object for sending signals during calls.
 *
 * \sa Client::call, Function::call
 */
class DFHACK_CLIENT_QT_EXPORT CallNotifier: public QObject
{
	Q_OBJECT
public:
	CallNotifier(QObject *parent = nullptr);
	~CallNotifier() override;

signals:
	/** Sent when the request message was sent (input can be re-used). */
	void started();
	/** Sent when the response message was received (output is ready). */
	void finished(DFHack::CommandResult);
	/** Sent when a text notification is received during a call. */
	void notification(DFHack::Color color, const QString &text);
};

/**
 * Notifier for bind operator.
 *
 * \sa Function::bind
 */
class DFHACK_CLIENT_QT_EXPORT BindNotifier: public QObject
{
	Q_OBJECT
public:
	BindNotifier(QObject *parent = nullptr);
	~BindNotifier() override;

signals:
	/**
	 * Signal sent when the bind operation finished.
	 *
	 * \arg success	the function was successfully bound.
	 */
	void bound(bool success);
};

} // namespace DFHack

#endif
