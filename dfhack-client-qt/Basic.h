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

#ifndef DFHACK_CLIENT_QT_DFHACK_BASIC_H
#define DFHACK_CLIENT_QT_DFHACK_BASIC_H

#include <dfhack-client-qt/Function.h>

#include <dfhack-client-qt/BasicApi.pb.h>

namespace DFHack
{

namespace Basic
{
	using GetVersion = Function<"", "GetVersion", dfproto::EmptyMessage, dfproto::StringMessage>;
	using GetDFVersion = Function<"", "GetDFVersion", dfproto::EmptyMessage, dfproto::StringMessage>;
	using GetWorldInfo = Function<"", "GetWorldInfo", dfproto::EmptyMessage, dfproto::GetWorldInfoOut>;
	using ListEnums = Function<"", "ListEnums", dfproto::EmptyMessage, dfproto::ListEnumsOut>;
	using ListJobSkills = Function<"", "ListJobSkills", dfproto::EmptyMessage, dfproto::ListJobSkillsOut>;
	using ListMaterials = Function<"", "ListMaterials", dfproto::ListMaterialsIn, dfproto::ListMaterialsOut>;
	using ListUnits = Function<"", "ListUnits", dfproto::ListUnitsIn, dfproto::ListUnitsOut>;
	using ListSquads = Function<"", "ListSquads", dfproto::ListSquadsIn, dfproto::ListSquadsOut>;
	using SetUnitLabors = Function<"", "SetUnitLabors", dfproto::SetUnitLaborsIn, dfproto::EmptyMessage>;
};

} // namespace DFHack

#endif
