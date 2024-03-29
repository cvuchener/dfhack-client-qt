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

#ifndef DFHACK_CLIENT_QT_DFHACK_FUNCTION_H
#define DFHACK_CLIENT_QT_DFHACK_FUNCTION_H

#include <dfhack-client-qt/Client.h>

namespace DFHack
{

/**
 * Convenience class for binding and calling remote module functions.
 *
 * In and Out are protocol buffers message types for parameters and results.
 *
 * Only set Id for functions with fixed ids, regular functions should leave
 * the default value.
 */
template<typename In, typename Out, int Id = -1>
class Function
{
	const std::string module;
	const std::string name;
	dfproto::CoreBindRequest bind_request;
	static constexpr int id = Id;
public:
	using InputMessage = In;
	using OutputMessage = Out;

	Function(std::string_view module, std::string_view name)
		: module(module)
		, name(name)
	{
		bind_request.set_method(this->name);
		bind_request.set_input_msg(InputMessage().GetTypeName());
		bind_request.set_output_msg(OutputMessage().GetTypeName());
		bind_request.set_plugin(this->module);
	}

	/**
	 * Create a message for input arguments.
	 */
	InputMessage args() const { return {}; }

	/**
	 * Bind the function.
	 *
	 * \returns a future boolean telling if the bind operation was successful.
	 */
	QFuture<bool> bind(Client &client) const requires (id == -1)
	{
		return getBinding(client)->result.then([](CommandResult cr) {
			return cr == CommandResult::Ok;
		});
	}

	/**
	 * Call the function.
	 *
	 * If the function is not already bound, it will be bound before the
	 * actual call is sent.
	 *
	 * When the future are started \ref in is reusable, after they are
	 * finished with a CommandResult::Ok value, \ref out is set to
	 * function results.
	 *
	 * For a given Function object, \ref call must not be called again
	 * before the previous call is finished.
	 *
	 * \returns a pair of future command result and future text notifications,
	 * if the command result is CommandResult::Ok, \ref out is ready.
	 */
	std::pair<QFuture<CallReply<OutputMessage>>, QFuture<TextNotification>>
	operator()(Client &client, const InputMessage &in = {}) const
	{
		std::pair<QFuture<CallReply<>>, QFuture<TextNotification>> res;
		if constexpr (id == -1)
			res = client.call(getBinding(client), in, std::make_shared<Out>());
		else
			res = client.call(id, in, std::make_shared<Out>());
		return {
			res.first.then([](CallReply<> r) { return std::move(r).cast<OutputMessage>(); }),
			res.second
		};
	}

private:
	std::shared_ptr<Client::Binding> getBinding(Client &client) const
	{
		return client.getBinding(bind_request);
	}
};

template <typename T>
concept BindableFunction = requires (T f) {
	{ f.bind() } -> std::same_as<QFuture<bool>>;
};

/**
 * Bind all the functions passed as parameters.
 *
 * \returns a future boolean indicating if all the functions where bound
 * successfully.
 */
template <BindableFunction... Fs>
QFuture<bool> bindAll(Fs &&...functions)
{
	QList<QFuture<bool>> futures;
	(futures.append(functions.bind()), ...);
	return QtFuture::whenAll(futures.begin(), futures.end()).then([](const QList<QFuture<bool>> &r) {
		return std::ranges::all_of(r, &QFuture<bool>::result<bool>);
	});
}

}

#endif
