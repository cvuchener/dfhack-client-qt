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

#include <dfhack-client-qt/globals.h>
#include <dfhack-client-qt/BasicApi.pb.h>

namespace DFHack
{

struct Basic
{
	const Function<dfproto::EmptyMessage, dfproto::StringMessage> getVersion = {"", "GetVersion"};
	const Function<dfproto::EmptyMessage, dfproto::StringMessage> getDFVersion = {"", "GetDFVersion"};
	const Function<dfproto::EmptyMessage, dfproto::GetWorldInfoOut> getWorldInfo = {"", "GetWorldInfo"};
	const Function<dfproto::EmptyMessage, dfproto::ListEnumsOut> listEnums = {"", "ListEnums"};
	const Function<dfproto::EmptyMessage, dfproto::ListJobSkillsOut> listJobSkills = {"", "ListJobSkills"};
	const Function<dfproto::ListMaterialsIn, dfproto::ListMaterialsOut> listMaterials = {"", "ListMaterials"};
	const Function<dfproto::ListUnitsIn, dfproto::ListUnitsOut> listUnits = {"", "ListUnits"};
	const Function<dfproto::ListSquadsIn, dfproto::ListSquadsOut> listSquads = {"", "ListSquads"};
	const Function<dfproto::SetUnitLaborsIn, dfproto::EmptyMessage> setUnitLabors = {"", "SetUnitLabors"};
};

} // namespace DFHack

#endif
