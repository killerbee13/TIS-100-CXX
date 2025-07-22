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
#ifndef SCORE_HPP
#define SCORE_HPP

#include "utils.hpp"

#include <cstddef>
#include <string>

struct score {
	std::size_t cycles{};
	std::size_t nodes{};
	std::size_t instructions{};
	bool validated{};
	bool achievement{};
	bool cheat{};
	bool hardcoded{};

	friend std::string to_string(score sc, bool colored = color_stdout) {
		std::string ret;
		if (sc.validated) {
			append(ret, sc.cycles);
		} else {
			if (colored) {
				ret += escape_code(red);
			}
			ret += "-";
		}
		append(ret, '/', sc.nodes, '/', sc.instructions);
		if (sc.validated) {
			if (sc.achievement or sc.cheat) {
				ret += '/';
			}
			if (sc.achievement) {
				if (colored) {
					ret += escape_code(bright_blue, bold);
				}
				ret += 'a';
				if (colored) {
					ret += escape_code(none);
				}
			}
			if (sc.hardcoded) {
				if (colored) {
					ret += escape_code(red);
				}
				ret += 'h';
			} else if (sc.cheat) {
				if (colored) {
					ret += escape_code(yellow);
				}
				ret += 'c';
			}
		}
		if (colored) {
			ret += escape_code(none);
		}
		return ret;
	}
};

#endif // SCORE_HPP
