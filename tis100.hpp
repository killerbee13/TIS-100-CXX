/* *****************************************************************************
 * TIS-100-CXX
 * Copyright (c) 2025 killerbee, Andrea Stacchiotti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ****************************************************************************/
#ifndef TIS100_HPP
#define TIS100_HPP

#include "utils.hpp"

/** @file tis100.hpp
 * Fundamental TIS-100 values and types
 */

/// Support a 3D expansion in node connections
constexpr inline uint DIMENSIONS = 2;

namespace defaults {
constexpr inline uint T21_size = 15;
constexpr inline uint T30_size = 15;
constexpr inline size_t cycles_limit = 150'000;
constexpr inline size_t total_cycles_limit = kblib::max;
constexpr inline bool run_fixed = true;
constexpr inline uint num_threads = 1;
constexpr inline double cheat_rate = 0.05;
constexpr inline double limit_multiplier = 5.0;
} // namespace defaults

enum port : std::int8_t {
	left,
	right,
	up,
	down,
	D5, // 3D expansion
	D6,

	// The directions are values 0-N, so < and > can be used to compare them
	dir_first = 0,
	dir_last = DIMENSIONS * 2 - 1,

	nil = std::max(D6, dir_last) + 1,
	acc,
	any,
	last,
	immediate = -1
};

constexpr port& operator++(port& p) {
	assert(p >= port::dir_first and p <= port::dir_last);
	p = static_cast<port>(std::to_underlying(p) + 1);
	return p;
}
constexpr port operator++(port& p, int) {
	port old = p;
	++p;
	return old;
}

constexpr port invert(port p) {
	assert(p >= port::dir_first and p <= port::dir_last);
	return static_cast<port>(std::to_underlying(p) ^ 1);
}

constexpr static std::string_view port_name(port p) {
	switch (p) {
	case left:
		return "LEFT";
	case right:
		return "RIGHT";
	case up:
		return "UP";
	case down:
		return "DOWN";
	case D5:
		return "D5";
	case D6:
		return "D6";
	case nil:
		return "NIL";
	case acc:
		return "ACC";
	case any:
		return "ANY";
	case last:
		return "LAST";
	case immediate:
		return "VAL";
	default:
		throw std::invalid_argument{concat("Illegal port ", etoi(p))};
	}
}

enum class node_type_t : int8_t {
	T21 = 1,
	T30,
	in,
	out,
	image,
	Damaged = -1,
	null = -2
};

#endif // TIS100_HPP
