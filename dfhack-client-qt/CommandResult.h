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

#ifndef DFHACK_CLIENT_QT_COMMAND_RESULT_H
#define DFHACK_CLIENT_QT_COMMAND_RESULT_H

#include <QMetaType>
#include <dfhack-client-qt/globals.h>

namespace DFHack
{

enum class CommandResult: int32_t
{
	LinkFailure = -3,
	NeedsConsole = -2,
	NotImplemented = -1,
	Ok = 0,
	Failure = 1,
	WrongUsage = 2,
	NotFound = 3,
};

DFHACK_CLIENT_QT_EXPORT const std::error_category &command_result_category();

DFHACK_CLIENT_QT_EXPORT std::error_code make_error_code(CommandResult cr);

} // namespace DFHack

Q_DECLARE_METATYPE(DFHack::CommandResult);

template<>
struct std::is_error_code_enum<DFHack::CommandResult>: std::true_type {};

#endif
