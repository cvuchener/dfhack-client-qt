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

#include <dfhack-client-qt/CommandResult.h>

using namespace DFHack;

class dfhack_command_result_category: public std::error_category
{
public:
	const char *name() const noexcept
	{
		return "dfhack_command_result";
	}

	std::string message(int condition) const
	{
		using namespace std::literals;
		switch (static_cast<CommandResult>(condition)) {
		case CommandResult::LinkFailure:
			return "Link failure"s;
		case CommandResult::NeedsConsole:
			return "Needs console"s;
		case CommandResult::NotImplemented:
			return "Not implemented"s;
		case CommandResult::Ok:
			return "Ok"s;
		case CommandResult::Failure:
			return "Failure"s;
		case CommandResult::WrongUsage:
			return "Wrong usage"s;
		case CommandResult::NotFound:
			return "Not found"s;
		default:
			return "unknown error code"s;
		}
	}
};

const std::error_category &DFHack::command_result_category()
{
	static const dfhack_command_result_category category;
	return category;
}

std::error_code DFHack::make_error_code(CommandResult cr)
{
	return {static_cast<int>(cr), command_result_category()};
}
