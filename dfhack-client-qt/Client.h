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
#include <dfhack-client-qt/CommandResult.h>
#include <dfhack-client-qt/CoreProtocol.pb.h>
#include <dfhack-client-qt/static_string.h>

namespace DFHack
{

enum class Color
{
	Black = 0,
	Blue = 1,
	Green = 2,
	Cyan = 3,
	Red = 4,
	Magenta = 5,
	Brown = 6,
	Grey = 7,
	DarkGrey = 8,
	LightBlue = 9,
	LightGreen = 10,
	LightCyan = 11,
	LightRed = 12,
	LightMagenta = 13,
	Yellow = 14,
	White = 15
};

using TextNotification = std::pair<DFHack::Color, QString>;

/**
 * Reply to a function call
 *
 * It contains both the command result code \ref cr and (optionally) the reply
 * message \ref msg.
 *
 * \ref msg will null for failed calls.
 *
 * Default message type is the base MessageLite type. CallReply<> can be casted
 * to derived message types CallReply<MyFunctionReply>.
 */
template <typename T = google::protobuf::MessageLite>
struct CallReply {
	CommandResult cr;
	std::shared_ptr<T> msg;

	CallReply(CommandResult cr, std::shared_ptr<T> &&msg) noexcept
		: cr(cr)
		, msg(std::move(msg))
	{
	}

	CallReply(CommandResult cr) noexcept
		: cr(cr)
		, msg(nullptr)
	{
	}

	CallReply(std::shared_ptr<T> &&msg) noexcept
		: cr(CommandResult::Ok)
		, msg(std::move(msg))
	{
	}

	/**
	 * Checks if the call succeeded
	 */
	operator bool() const noexcept { return cr == CommandResult::Ok; }
	/**
	 * Access the reply message
	 */
	const T &operator*() const noexcept { return *msg; }
	/**
	 * Access the reply message members
	 */
	const T *operator->() const noexcept { return msg.get(); }

	template <typename U> requires std::same_as<T, google::protobuf::MessageLite>
	CallReply<U> cast() const & noexcept { return {cr, static_pointer_cast<U>(msg)}; }
	template <typename U> requires std::same_as<T, google::protobuf::MessageLite>
	CallReply<U> cast() && noexcept { return {cr, static_pointer_cast<U>(std::move(msg))}; }
};

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
	 * \returns a pair of future call reply and future text notifications.
	 * If the call succeeds, \ref CallReply<>::msg will contain \ref out.
	 */
	std::pair<QFuture<CallReply<>>, QFuture<TextNotification>> call(
			int16_t id,
			const google::protobuf::MessageLite &in,
			std::shared_ptr<google::protobuf::MessageLite> out);

	struct Binding
	{
		/**
		 * Call id. Content is valid only if \ref result is finished
		 * and not an error.
		 *
		 * Use assigned_id to get the call id.
		 */
		int id;
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

	std::pair<QFuture<CallReply<>>, QFuture<TextNotification>> enqueueCall(
			int id,
			const google::protobuf::MessageLite *in,
			std::shared_ptr<google::protobuf::MessageLite> &&out);

	void sendNextCall();

	void readyRead();
	void connected();
	void disconnected();
	void error(QAbstractSocket::SocketError error);

	void finishConnection(bool success);
	void finishCall(CommandResult result);

	void invalidateBindings();
};

} // namespace DFHack

Q_DECLARE_METATYPE(DFHack::Color);

#endif
