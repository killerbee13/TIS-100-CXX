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
#include <string_view>

constexpr uint find_level_id(std::string_view s) {
	for (auto i : range(layouts.size())) {
		if (s == layouts[i].segment or s == layouts[i].name) {
			return static_cast<uint>(i);
		}
	}
	throw std::invalid_argument{concat("invalid level ID ", kblib::quoted(s))};
}

consteval uint operator""_lvl(const char* s, std::size_t size) {
	return find_level_id(std::string_view(s, size));
}

constexpr std::optional<uint> guess_level_id(std::string_view filename) {
	for (auto i : range(layouts.size())) {
		if (filename.starts_with(layouts[i].segment)) {
			return i;
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

inline bool check_achievement(uint level_id, const field& solve, score sc) {
	auto log = log_debug();
	log << "check_achievement " << layouts[level_id].name << ": ";
	// SELF-TEST DIAGNOSTIC
	if (level_id == "00150"_lvl) {
		// BUSY_LOOP
		log << "BUSY_LOOP: " << sc.cycles << ((sc.cycles > 100000) ? ">" : "<=")
		    << 100000;
		return sc.cycles > 100000;
		// SIGNAL COMPARATOR
	} else if (level_id == "21340"_lvl) {
		// UNCONDITIONAL
		log << "UNCONDITIONAL:\n";
		for (auto& n : solve.regulars()) {
			if (n->type() == node::T21) {
				auto p = static_cast<const T21*>(n.get());
				log << "T20 (" << p->x << ',' << p->y << "): ";
				if (p->code.empty()) {
					log << "empty";
				} else if (p->has_instr(instr::jez, instr::jnz, instr::jgz,
				                        instr::jlz)) {
					log << " conditional found";
					return false;
				}
				log << '\n';
			}
		}
		log << " no conditionals found";
		return true;
		// SEQUENCE REVERSER
	} else if (level_id == "42656"_lvl) {
		// NO_MEMORY
		log << "NO_MEMORY: ";
		for (auto& n : solve.regulars()) {
			if (n->type() == node::T30) {
				auto p = static_cast<const T30*>(n.get());
				log << "T30 (" << p->x << ',' << p->y << "): " << p->used << '\n';
				if (p->used) {
					return false;
				}
			}
		}
		log << "no stacks used";
		return true;
	} else {
		log << "no achievement";
		return false;
	}
}

#endif // BUILTIN_LEVELS_HPP
