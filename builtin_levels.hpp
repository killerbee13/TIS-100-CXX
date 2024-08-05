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
	auto log = log_debug();
	log << "check_achievement " << layouts[static_cast<std::size_t>(id)].name
	    << ": ";
	// SELF-TEST DIAGNOSTIC
	if (id == "00150"_lvl) {
		// BUSY_LOOP
		log << "BUSY_LOOP: " << sc.cycles << ((sc.cycles > 100000) ? ">" : "<=")
		    << 100000;
		return sc.cycles > 100000;
		// SIGNAL COMPARATOR
	} else if (id == "21340"_lvl) {
		// UNCONDITIONAL
		log << "UNCONDITIONAL:\n";
		for (std::size_t i = 0; i < solve.nodes_avail(); ++i) {
			if (auto p = solve.node_by_index(i)) {
				log << '@' << i << " T20 (" << p->x << ',' << p->y << "): ";
				if (p->code.empty()) {
					log << "empty";
				} else if (std::any_of(
				               p->code.begin(), p->code.end(), [&](const instr& i) {
					               auto op = i.get_op();
					               log << to_string(op) << ';';
					               return op == instr::jez or op == instr::jnz
					                      or op == instr::jgz or op == instr::jlz;
				               })) {
					log << " conditional found";
					return false;
				}
				log << '\n';
			}
		}
		log << " no conditionals found";
		return true;
		// SEQUENCE REVERSER
	} else if (id == "42656"_lvl) {
		// NO_MEMORY
		log << "NO_MEMORY: ";

		for (auto it = solve.begin(); it != solve.end_regular(); ++it) {
			auto p = it->get();
			if (p->type() == node::T30) {
				auto n = static_cast<const T30*>(p);
				log << "T30 (" << n->x << ',' << n->y << "): " << n->used << '\n';
				if (n->used) {
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
