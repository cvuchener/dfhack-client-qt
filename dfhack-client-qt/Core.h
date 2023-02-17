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

#ifndef DFHACK_CLIENT_QT_DFHACK_CORE_H
#define DFHACK_CLIENT_QT_DFHACK_CORE_H

#include <dfhack-client-qt/Function.h>

namespace DFHack
{

namespace Core
{
	using BindMethod = Function<"", "BindMethod", dfproto::CoreBindRequest, dfproto::CoreBindReply, 0>;
	using RunCommand = Function<"", "RunCommand", dfproto::CoreRunCommandRequest, dfproto::EmptyMessage, 1>;

	using Suspend = Function<"", "CoreSuspend", dfproto::EmptyMessage, dfproto::IntMessage>;
	using Resume = Function<"", "CoreResume", dfproto::EmptyMessage, dfproto::IntMessage>;

	using RunLua = Function<"", "RunLua", dfproto::CoreRunLuaRequest, dfproto::StringListMessage>;
};

} // namespace DFHack

#endif
