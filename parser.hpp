/* *****************************************************************************
 * TIX-100-CXX
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
#ifndef PARSER_HPP
#define PARSER_HPP

#include "T21.hpp"
#include "field.hpp"
#include "node.hpp"
#include "random_levels.hpp"

#include <string>
#include <vector>

/// Assemble a single node's code
std::vector<instr> assemble(std::string_view source, int node,
                            std::size_t T21_size = def_T21_size);
/// Read a TIS-100-compatible save file
/// @throws `std::invalid_argument` for any lexing problem
void parse_code(field& f, std::string_view source, std::size_t T21_size);
/// Configure the field with a test case
void set_expected(field& f, const single_test& expected);

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

#endif // PARSER_HPP
