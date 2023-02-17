/*
 * Copyright 2023 Clement Vuchener
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

#ifndef DFHACK_CLIENT_QT_DFHACK_CLIENT_H
#define DFHACK_CLIENT_QT_DFHACK_CLIENT_H

#include <QAbstractSocket>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QFuture>

#include <dfhack-client-qt/globals.h>
#include <dfhack-client-qt/CoreProtocol.pb.h>
#include <dfhack-client-qt/static_string.h>

namespace DFHack
{

using TextNotification = std::pair<DFHack::Color, QString>;

/**
 * DFHack remote protocol client
 */
class DFHACK_CLIENT_QT_EXPORT Client: public QObject
{
	Q_OBJECT
public:
	Client(QObject *parent = nullptr);
	~Client() override;

	static constexpr quint16 DefaultPort = 5000;

	/**
	 * Connect to DFHack server
	 *
	 * A connectionChanged or socketError signal will be emitted when
	 * actual connection is done.
	 *
	 * \returns a future indicating connection success.
	 */
	QFuture<bool> connect(const QString &host, quint16 port);
	/**
	 * Disconnect from DFHack server
	 *
	 * A connectionChanged signal is sent emitted when the socket is
	 * disconnected. A socketError may also be emitted if it does happen as
	 * expected by protocol.
	 *
	 * \return a future indicating when the disconnection is finished.
	 */
	QFuture<void> disconnect();

	/**
	 * Low-level remote function call
	 *
	 * Call function \p id with parameters \p in and stores results in \p out.
	 *
	 * \returns a pair of future command result and future text notifications,
	 * if the command result is CommandResult::Ok, \p out is ready.
	 */
	std::pair<QFuture<CommandResult>, QFuture<TextNotification>> call(
			int16_t id,
			const google::protobuf::MessageLite &in,
			google::protobuf::MessageLite &out);

	struct Binding
	{
		/**
		 * Reply to the bind request. Content is valid only if
		 * \ref result is finished and not an error.
		 *
		 * Use assigned_id to get the call id.
		 */
		dfproto::CoreBindReply reply;
		/**
		 * Result for the bind request.
		 */
		QFuture<CommandResult> result;

		/**
		 * Check if reply is valid and can be used.
		 */
		bool ready() const
		{
			return result.isValid()
				&& result.isFinished()
				&& result.result() == CommandResult::Ok;
		}
	};
	/**
	 * Get a binding from a bind request. Bindings are cached so they are
	 * actually only requested once. Bindings are invalidated when the
	 * connection is lost.
	 */
	std::shared_ptr<Binding> getBinding(const dfproto::CoreBindRequest &);
signals:
	/**
	 * Signal emitted when the client is connected or disconnected.
	 */
	void connectionChanged(bool connected);
	/**
	 * Signal emitted when the client socket emits errors.
	 */
	void socketError(QAbstractSocket::SocketError error, const QString &error_string);
	/**
	 * Signal emitted when a text notification is received.
	 */
	void notification(DFHack::Color color, const QString &text);

private:
	struct Private;
	std::unique_ptr<Private> p;

	std::pair<QFuture<CommandResult>, QFuture<TextNotification>> enqueueCall(
			int id,
			const google::protobuf::MessageLite *in,
			google::protobuf::MessageLite *out);

	void sendNextCall();

	void readyRead();
	void connected();
	void disconnected();
	void error(QAbstractSocket::SocketError error);

	void finishConnection(bool success);
	void finishCall(CommandResult result);

	template<typename T> bool read(T *data);
	bool read(QByteArray &data, qint64 size);
	template<typename T> bool write(const T *data);
	bool write(const char *data, qint64 size);

	void invalidateBindings();
};

} // namespace DFHack

#endif
