/* *****************************************************************************
 * %{QMAKE_PROJECT_NAME}
 * Copyright (c) %YEAR% killerbee
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
#ifndef RANDOM_LEVELS_HPP
#define RANDOM_LEVELS_HPP

#include "builtin_levels.hpp"
#include <kblib/random.h>

inline auto rng = kblib::seeded<kblib::common_lcgs::rand48>();

std::vector<word_t> random_inputs(word_t low, word_t high,
                                  std::size_t length = 39) {
	std::vector<word_t> ret;
	std::uniform_int_distribution<word_t> i(0, 99);
	auto r = [&] -> word_t { return i(rng); };
	ret.resize(length);
	std::ranges::generate(ret, r);
	return ret;
}

single_test random_test(int id) {
	single_test ret{};
	switch (id) {
	case "SELF-TEST DIAGNOSTIC"_lvl: {
		ret.inputs.push_back(random_inputs(0, 99));
		ret.inputs.push_back(random_inputs(0, 99));
		ret.n_outputs = ret.inputs;
	} break;
	case "SIGNAL AMPLIFIER"_lvl: {
		ret.inputs.push_back(random_inputs(0, 99));
		ret.n_outputs.resize(1);
		std::ranges::transform(ret.inputs[0],
		                       std::back_inserter(ret.n_outputs[0]),
		                       [](word_t x) { return 2 * x; });
	} break;
	case "DIFFERENTIAL CONVERTER"_lvl: {
		ret.inputs.push_back(random_inputs(0, 99));
		ret.inputs.push_back(random_inputs(0, 99));
		ret.n_outputs.resize(2);
		std::ranges::transform(ret.inputs[0], ret.inputs[1],
		                       std::back_inserter(ret.n_outputs[0]),
		                       [](word_t x, word_t y) { return x - y; });
		std::ranges::transform(ret.inputs[0], ret.inputs[1],
		                       std::back_inserter(ret.n_outputs[0]),
		                       [](word_t x, word_t y) { return y - x; });
	} break;
	case "SIGNAL COMPARATOR"_lvl: {
		ret.inputs.push_back(random_inputs(-2, 2));
		ret.n_outputs.resize(3, std::vector<word_t>(39));
		for (auto [x, i] : kblib::enumerate(ret.inputs[0])) {
			ret.n_outputs[0][i] = (x > 0);
			ret.n_outputs[1][i] = (x == 0);
			ret.n_outputs[2][i] = (x < 0);
		}
	} break;
	case "SIGNAL MULTIPLEXER"_lvl: {
		ret.inputs.resize(3);
		ret.n_outputs.resize(1, std::vector<word_t>(39));
		ret.inputs[0] = random_inputs(-99, 0);
		ret.inputs[1] = random_inputs(-1, 1);
		ret.inputs[2] = random_inputs(0, 99);
		for (auto [x, i] : kblib::enumerate(ret.inputs[1])) {
			if (x <= 0) {
				x += ret.inputs[0][i];
			}
			if (x >= 0) {
				x += ret.inputs[0][i];
			}
		}
	} break;
	case "SEQUENCE GENERATOR"_lvl: {
		ret.inputs.push_back(random_inputs(0, 99, 13));
		ret.inputs.push_back(random_inputs(0, 99, 13));
		ret.n_outputs.resize(1);
		for (const auto i : kblib::range(13)) {
			ret.n_outputs[0].push_back(ret.inputs[0][i]);
			ret.n_outputs[0].push_back(ret.inputs[1][i]);
			ret.n_outputs[0].push_back(0);
		}
	} break;
	case "SEQUENCE COUNTER"_lvl: {
		ret.inputs.push_back(random_inputs(1, 99));
		ret.inputs[0][38] = 0;
		std::uniform_int_distribution<word_t> dst(8, 12);
		auto c = dst(rng);
		dst.param(std::uniform_int_distribution<word_t>::param_type{0, 38});
		for (const auto n : kblib::range(c)) {
			ret.inputs[0][n] = 0;
		}

		word_t sum{};
		word_t count{};
		ret.n_outputs.resize(2);
		for (auto w : ret.inputs[0]) {
			if (w == 0) {
				ret.n_outputs[0].push_back(sum);
				ret.n_outputs[1].push_back(count);
			} else {
				++count;
				sum += w;
			}
		}
	} break;
	case "SIGNAL EDGE DETECTOR"_lvl: {
		ret.inputs.push_back(random_inputs(-20, 120));
		ret.inputs[0][0] = 0;
		ret.n_outputs.resize(1);
		word_t prev = 0;
		for (auto w : ret.inputs[0]) {
			ret.n_outputs[0].push_back(std::abs(w - std::exchange(prev, w)) >= 10);
		}
	} break;
	case "INTERRUPT HANDLER"_lvl: {
		ret.inputs.resize(4, std::vector<word_t>(39));
		ret.n_outputs.resize(1, std::vector<word_t>(39));
		std::uniform_int_distribution<word_t> d(0, 3);
		for (const auto i : kblib::range(1, 39)) {
			for (const auto j : kblib::range(4)) {
				ret.inputs[j][i] = ret.inputs[j][i - 1];
			}
			if (d(rng) > 1) {
				auto k = d(rng);
				if ((ret.inputs[k][i] = not ret.inputs[k][i - 1])) {
					ret.n_outputs[k][i] = k;
				}
			}
		}
	} break;
	}
	return ret;
}

#endif // RANDOM_LEVELS_HPP
