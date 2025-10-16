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
#ifndef TESTS_HPP
#define TESTS_HPP

#include "image.hpp"
#include "utils.hpp"

constexpr inline word_t image_width = 30;
constexpr inline word_t image_height = 18;
constexpr inline int max_test_length = 39;

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

#endif // TESTS_HPP
