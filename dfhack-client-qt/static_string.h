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

#ifndef DFHACK_CLIENT_QT_DFHACK_STATIC_STRING_H
#define DFHACK_CLIENT_QT_DFHACK_STATIC_STRING_H

#include <ranges>

namespace DFHack
{

template <std::size_t N>
struct static_string
{
	char data[N];
	constexpr static_string(const char (&str)[N]) {
		std::ranges::copy(str, data);
	}

	constexpr operator std::string_view() const {
		return {data, N-1};
	}

	constexpr std::string_view str() const {
		return {data, N-1};
	}

	constexpr const char *c_str() const {
		return data;
	}

	constexpr auto begin() const { return &data[0]; }
	constexpr auto end() const { return &data[N-1]; }
};

}

#endif
