/* *****************************************************************************
 * TIS-100-CXX
 * Copyright (c) 2024 killerbee, Andrea Stacchiotti
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
#ifndef BUILTIN_LEVELS_HPP
#define BUILTIN_LEVELS_HPP

#include "T30.hpp"
#include "layoutspecs.hpp"
#include "parser.hpp"

#include <array>
#include <ranges>
#include <string_view>

constexpr int level_id(std::string_view s) {
	for (auto i : range(layouts.size())) {
		if (s == layouts[i].segment or s == layouts[i].name) {
			return static_cast<int>(i);
		}
	}
	throw std::invalid_argument{concat("invalid level ID ", kblib::quoted(s))};
}

consteval int operator""_lvl(const char* s, std::size_t size) {
	return level_id(std::string_view(s, size));
}

constexpr std::optional<int> guess_level_id(std::string_view filename) {
	for (auto i : range(layouts.size())) {
		if (filename.starts_with(layouts[i].segment)) {
			return static_cast<int>(i);
		}
	}
	return std::nullopt;
}

inline image_t checkerboard(std::ptrdiff_t w, std::ptrdiff_t h) {
	image_t ret(w, h);
	for (const auto y : range(h)) {
		for (const auto x : range(w)) {
			ret.at(x, y)
			    = ((x + y % 2) % 2) ? tis_pixel::C_black : tis_pixel::C_white;
		}
	}
	return ret;
}

inline bool check_achievement(int id, const field& solve, score sc) {
	auto debug = log_debug();
	debug << "check_achievement " << layouts[static_cast<std::size_t>(id)].name
	    << ": ";
	// SELF-TEST DIAGNOSTIC
	if (id == "00150"_lvl) {
		// BUSY_LOOP
		debug << "BUSY_LOOP: " << sc.cycles << ((sc.cycles > 100000) ? ">" : "<=")
		    << 100000;
		return sc.cycles > 100000;
		// SIGNAL COMPARATOR
	} else if (id == "21340"_lvl) {
		// UNCONDITIONAL
		debug << "UNCONDITIONAL:\n";
		for (auto [i, n] : std::views::enumerate(solve.nodes_T21)) {
			debug << '@' << i << " T20 (" << n.x << ',' << n.y << "): ";
			if (n.code.empty()) {
				debug << "empty";
			} else if (std::any_of(
							n.code.begin(), n.code.end(), [&](const instr& i) {
								auto op = i.op_;
								debug << to_string(op) << ';';
								return op == instr::jez or op == instr::jnz
										or op == instr::jgz or op == instr::jlz;
							})) {
				debug << " conditional found";
				return false;
			}
			debug << '\n';
		}
		debug << " no conditionals found";
		return true;
		// SEQUENCE REVERSER
	} else if (id == "42656"_lvl) {
		// NO_MEMORY
		debug << "NO_MEMORY: ";

		for (auto& n: solve.nodes_T30) {
			debug << "T30 (" << n.x << ',' << n.y << "): " << n.used << '\n';
			if (n.used) {
				return false;
			}
		}
		debug << "no stacks used";
		return true;
	} else {
		debug << "no achievement";
		return false;
	}
}

#endif // BUILTIN_LEVELS_HPP
