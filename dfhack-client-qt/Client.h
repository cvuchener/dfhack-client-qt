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

#include <future>

#include <dfhack-client-qt/globals.h>
#include <dfhack-client-qt/Notifier.h>
#include <dfhack-client-qt/CoreProtocol.pb.h>

namespace DFHack
{

template<const char *, const char *, typename, typename, int = -1>
class Function;

class ClientPrivate;

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
	 * \returns false if the client is already connected, true if a
	 * connection attempt was started.
	 *
	 * A connectionChanged or socketError signal will be emitted when
	 * actual connection is done.
	 */
	bool connect(const QString &host, quint16 port);
	/**
	 * Disconnect from DFHack server
	 *
	 * \returns false if the client is not connected, true if the quit
	 * request was sent.
	 *
	 * A connectionChanged signal is sent emitted when the socket is
	 * disconnected. A socketError may also be emitted if it does happen as
	 * expected by protocol.
	 */
	bool disconnect();

	/**
	 * Low-level remote function call
	 *
	 * Call function \p id with parameters \p in and stores results in \p
	 * out. If \p notifier is not null, signals are emitted according to
	 * the function call progress.
	 *
	 * \returns a future command result, if it is set to CommandResult::Ok,
	 * \p out is ready.
	 */
	std::future<CommandResult> call(
			int16_t id,
			const google::protobuf::MessageLite *in,
			google::protobuf::MessageLite *out,
			CallNotifier *notifier = nullptr);

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
	QThread thread;
	std::unique_ptr<ClientPrivate> p;

	struct Binding
	{
		dfproto::CoreBindReply reply;
		CallNotifier notifier;
		bool valid;
		std::promise<bool> promise;
		std::shared_future<bool> future;

		Binding()
			: valid(true)
			, future(promise.get_future())
		{
		}

		bool ready() const
		{
			auto status = future.wait_for(std::chrono::seconds(0));
			return status == std::future_status::ready;
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
	std::shared_future<bool> bind(const std::string &plugin, const std::string &name,
				      std::shared_ptr<Binding> &binding,
				      BindNotifier *notifier = nullptr)
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
			QObject::connect(&it->second->notifier, &CallNotifier::finished,
					[promise = &it->second->promise] (CommandResult result)
					{ promise->set_value(result == CommandResult::Ok); });
			call(0, &it->first, &it->second->reply, &it->second->notifier);
		}
		binding = it->second;
		if (notifier) {
			if (it->second->ready())
				emit notifier->bound(it->second->future.get());
			else
				QObject::connect(&it->second->notifier, &CallNotifier::finished,
						[notifier] (CommandResult result)
						{ emit notifier->bound(result == CommandResult::Ok); });
		}
		return it->second->future;
	}

	template<const char *Module, const char *Name, typename In, typename Out, int id>
	friend class Function;
};

} // namespace DFHack

#endif
