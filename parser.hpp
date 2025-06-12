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
#include "image.hpp"
#include "node.hpp"
#include "utils.hpp"

#include <string>
#include <vector>

struct single_test {
	std::vector<word_vec> inputs{};
	std::vector<word_vec> n_outputs{};
	std::vector<image_t> i_outputs{};
};

inline void clamp_test_values(single_test& t) {
	auto debug = log_debug();
	// The game clamps negative values to -99 to fit in the 3 columns UI, but
	// it breaks tests (segment 32050 seed 103061) so we're doing the sensible
	// thing even if it's wrong as far as a simulational versimilitude is
	// concerned
	auto clamp_vec = [&](auto& vec) {
		write_list(debug, vec);
		std::ranges::transform(vec, vec.begin(), [](word_t v) {
			return std::clamp(v, word_min, word_max);
		});
		debug << " to ";
		write_list(debug, vec);
		debug << '\n';
	};
	for (auto& v : t.inputs) {
		debug << "Clamping in: ";
		clamp_vec(v);
	}
	for (auto& v : t.n_outputs) {
		debug << "Clamping out: ";
		clamp_vec(v);
	}
}

constexpr inline word_t image_width = 30;
constexpr inline word_t image_height = 18;
constexpr inline int max_test_length = 39;

/// Assemble a single node's code
std::vector<instr> assemble(std::string_view source, int node,
                            std::size_t T21_size = def_T21_size);
/// Read a TIS-100-compatible save file
/// @throws `std::invalid_argument` for any lexing problem
void parse_code(field& f, std::string_view source, std::size_t T21_size);
/// Configure the field with a test case, takes ownership of the test content
void set_expected(field& f, single_test&& expected);

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
