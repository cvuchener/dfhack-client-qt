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

#ifndef DFHACK_CLIENT_QT_GLOBALS_H
#define DFHACK_CLIENT_QT_GLOBALS_H

#include <QtCore/QtGlobal>

#if defined(DFHACK_CLIENT_QT_LIBRARY)
#	define DFHACK_CLIENT_QT_EXPORT Q_DECL_EXPORT
#else
#	define DFHACK_CLIENT_QT_EXPORT Q_DECL_IMPORT
#endif

#endif
