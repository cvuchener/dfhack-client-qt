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
 * the default value and call bind before any call.
 */
template<static_string Module, static_string Name, typename In, typename Out, int Id = -1>
class Function
{
	Client *client;
	std::shared_ptr<Client::Binding> binding;
public:
	static inline constexpr std::string_view module = Module;
	static inline constexpr std::string_view name = Name;
	static inline constexpr int id = Id;

	/**
	 * Input parameters
	 */
	In in;
	/**
	 * Output results
	 */
	Out out;

	Function(Client *client)
		: client(client)
	{
	}

	/**
	 * true if the function is ready to be called (bound or fixed id).
	 */
	operator bool() const
	{
		return id != -1 || isBound();
	}

	bool isBound() const
	{
		return binding && binding->ready();
	}

	/**
	 * Bind the function.
	 *
	 * Function that do not have a fixed id must be bound before any call.
	 *
	 * \returns a future boolean telling if the bind operation was successful.
	 */
	QFuture<bool> bind()
	{
		dfproto::CoreBindRequest request;
		request.set_method(Name.c_str());
		// descriptor is not available for lite messages.
		//request.set_input_msg(In::descriptor()->name());
		//request.set_output_msg(Out::descriptor()->name());
		request.set_input_msg(In().GetTypeName());
		request.set_output_msg(Out().GetTypeName());
		request.set_plugin(Module.c_str());
		binding = client->getBinding(request);
		return binding->result.then([](CommandResult cr){return cr == CommandResult::Ok;});
	}

	/**
	 * Call the function.
	 *
	 * \ref in parameters must be initialized before calling this function.
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
	std::pair<QFuture<CommandResult>, QFuture<TextNotification>> call()
	{
		if constexpr (id == -1) {
			if (!isBound()) {
				QPromise<CommandResult> p;
				p.addResult(CommandResult::LinkFailure);
				p.finish();
				return {p.future(), {}};
			}
			return client->call(binding->reply.assigned_id(), in, out);
		}
		else
			return client->call(id, in, out);
	}
};

}

#endif
