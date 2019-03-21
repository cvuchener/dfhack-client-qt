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

#ifndef DFHACK_CLIENT_QT_DFHACK_CORE_H
#define DFHACK_CLIENT_QT_DFHACK_CORE_H

#include <dfhack-client-qt/Function.h>

namespace DFHack
{

class Core
{
	static constexpr char module[] = "";
	static constexpr char bind[] = "CoreBind";
	static constexpr char run_command[] = "CoreRunCommand";
	static constexpr char suspend[] = "CoreSuspend";
	static constexpr char resume[] = "CoreResume";
public:
	using Bind = Function<module, bind, dfproto::CoreBindRequest, dfproto::CoreBindReply, 0>;
	using RunCommand = Function<module, run_command, dfproto::CoreRunCommandRequest, dfproto::EmptyMessage, 1>;
	using Suspend = Function<module, suspend, dfproto::EmptyMessage, dfproto::IntMessage>;
	using Resume = Function<module, resume, dfproto::EmptyMessage, dfproto::IntMessage>;
};

} // namespace DFHack

#endif
