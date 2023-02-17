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

#ifndef DFHACK_CLIENT_QT_DFHACK_CLIENT_H
#define DFHACK_CLIENT_QT_DFHACK_CLIENT_H

#include <QAbstractSocket>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QFuture>

#include <dfhack-client-qt/globals.h>
#include <dfhack-client-qt/CoreProtocol.pb.h>

namespace DFHack
{

template<const char *, const char *, typename, typename, int = -1>
class Function;

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
	 */
	void connect(const QString &host, quint16 port);
	/**
	 * Disconnect from DFHack server
	 *
	 * A connectionChanged signal is sent emitted when the socket is
	 * disconnected. A socketError may also be emitted if it does happen as
	 * expected by protocol.
	 */
	void disconnect();

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

signals:
	/**
	 * Signal emitted when the client is connected or disconnected.
	 */
	void connectionChanged(bool connected);
	/**
	 * Signal emitted when the client socket emits errors.
	 */
	void socketError(QAbstractSocket::SocketError error, const QString &error_string);

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

	void finishCall(CommandResult result);

	template<typename T> bool read(T *data);
	bool read(QByteArray &data, qint64 size);
	template<typename T> bool write(const T *data);
	bool write(const char *data, qint64 size);

	void invalidateBindings();

	struct Binding
	{
		dfproto::CoreBindReply reply;
		QFuture<CommandResult> result;

		bool ready() const
		{
			return result.isValid()
				&& result.isFinished()
				&& result.result() == CommandResult::Ok;
		}
	};

	static auto bind_request_to_tuple(const dfproto::CoreBindRequest &br)
	{
		return std::tie(br.plugin(), br.method(), br.input_msg(), br.output_msg());
	}
	struct bind_request_less {
		bool operator()(const dfproto::CoreBindRequest &lhs,
				const dfproto::CoreBindRequest &rhs) const
		{
			return bind_request_to_tuple(lhs) < bind_request_to_tuple(rhs);
		}
	};
	static bool is_same_bind_request(const dfproto::CoreBindRequest &lhs,
					 const dfproto::CoreBindRequest &rhs)
	{
		return bind_request_to_tuple(lhs) == bind_request_to_tuple(rhs);
	}
	std::map<dfproto::CoreBindRequest, std::shared_ptr<Binding>, bind_request_less> bindings;
	QMutex bindings_mutex;

	template<typename In, typename Out>
	QFuture<bool> bind(const std::string &plugin, const std::string &name,
				      std::shared_ptr<Binding> &binding)
	{
		QMutexLocker lock(&bindings_mutex);
		dfproto::CoreBindRequest request;
		request.set_method(name);
		// descriptor is not available for lite messages.
		//request.set_input_msg(In::descriptor()->name());
		//request.set_output_msg(Out::descriptor()->name());
		request.set_input_msg(In().GetTypeName());
		request.set_output_msg(Out().GetTypeName());
		request.set_plugin(plugin);

		auto it = bindings.lower_bound(request);
		if (it == bindings.end() || !is_same_bind_request(it->first, request)) {
			it = bindings.emplace_hint(it, request, std::make_shared<Binding>());
			it->second->result = call(0, it->first, it->second->reply).first;
		}
		binding = it->second;
		return it->second->result.then([](CommandResult cr){return cr == CommandResult::Ok;});
	}

	static QFuture<CommandResult> makeFailedResult();

	template<const char *Module, const char *Name, typename In, typename Out, int id>
	friend class Function;
};

} // namespace DFHack

#endif
