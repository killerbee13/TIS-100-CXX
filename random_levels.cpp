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

#include "random_levels.hpp"
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
					ret.n_outputs[0][i] = k;
				}
			}
		}
	} break;
	case "SIMPLE SANDBOX"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "SIGNAL PATTERN DETECTOR"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "SEQUENCE PEAK DETECTOR"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(2);
	} break;
	case "SEQUENCE REVERSER"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "SIGNAL MULTIPLIER"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "STACK MEMORY SANDBOX"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "IMAGE TEST PATTERN 1"_lvl: {
		ret.i_output.reshape(30, 18);
		for (auto& p : ret.i_output) {
			p = tis_pixel{tis_pixel::color::C_white};
		}
	} break;
	case "IMAGE TEST PATTERN 2"_lvl: {
		ret.i_output.reshape(30, 18);
		for (const auto x : kblib::range(ret.i_output.width())) {
			for (const auto y : kblib::range(ret.i_output.height())) {
				ret.i_output.at(x, y)
				    = ((x + y % 2) % 2) ? tis_pixel::C_black : tis_pixel::C_white;
			}
		}
	} break;
	case "EXPOSURE MASK VIEWER"_lvl: {
		ret.inputs.resize(1);
		ret.i_output.reshape(30, 18);
	} break;
	case "HISTOGRAM VIEWER"_lvl: {
		ret.inputs.resize(1);
		ret.i_output.reshape(30, 18);
	} break;
	case "IMAGE CONSOLE SANDBOX"_lvl: {
		ret.inputs.resize(1);
		ret.i_output.reshape(36, 22);
	} break;
	case "SIGNAL WINDOW FILTER"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(2);
	} break;
	case "SIGNAL DIVIDER"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(2);
	} break;
	case "SEQUENCE INDEXER"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "SEQUENCE SORTER"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "STORED IMAGE DECODER"_lvl: {
		ret.inputs.resize(1);
		ret.i_output.reshape(30, 18);
	} break;
	case "SEQUENCE MERGER"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "INTEGER SERIES CALCULATOR"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "SEQUENCE RANGE LIMITER"_lvl: {
		ret.inputs.resize(3);
		ret.n_outputs.resize(1);
	} break;
	case "SIGNAL ERROR CORRECTOR"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(2);
	} break;
	case "SUBSEQUENCE EXTRACTOR"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "SIGNAL PRESCALER"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(3);
	} break;
	case "SIGNAL AVERAGER"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "SUBMAXIMUM SELECTOR"_lvl: {
		ret.inputs.resize(4);
		ret.n_outputs.resize(1);
	} break;
	case "DECIMAL DECOMPOSER"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(3);
	} break;
	case "SEQUENCE MODE CALCULATOR"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "SEQUENCE NORMALIZER"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "IMAGE TEST PATTERN 3"_lvl: {
		ret.i_output.assign({
		    u"██████████████████████████████",
		    u"█                            █",
		    u"█ ██████████████████████████ █",
		    u"█ █                        █ █",
		    u"█ █ ██████████████████████ █ █",
		    u"█ █ █                    █ █ █",
		    u"█ █ █ ██████████████████ █ █ █",
		    u"█ █ █ █                █ █ █ █",
		    u"█ █ █ █ ██████████████ █ █ █ █",
		    u"█ █ █ █ ██████████████ █ █ █ █",
		    u"█ █ █ █                █ █ █ █",
		    u"█ █ █ ██████████████████ █ █ █",
		    u"█ █ █                    █ █ █",
		    u"█ █ ██████████████████████ █ █",
		    u"█ █                        █ █",
		    u"█ ██████████████████████████ █",
		    u"█                            █",
		    u"██████████████████████████████",
		});
	} break;
	case "IMAGE TEST PATTERN 4"_lvl: {
		ret.i_output.assign({
		    u" ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░",
		    u"░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ ",
		    u"▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█",
		    u"█▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒",
		    u" ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░",
		    u"░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ ",
		    u"▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█",
		    u"█▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒",
		    u" ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░",
		    u"░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ ",
		    u"▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█",
		    u"█▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒",
		    u" ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░",
		    u"░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ ",
		    u"▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█",
		    u"█▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒",
		    u" ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░▒█ ░",
		    u"░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ █▒░ ",
		});
	} break;
	case "SPATIAL PATH VIEWER"_lvl: {
		ret.inputs.resize(1);
		ret.i_output.reshape(30, 18);
	} break;
	case "CHARACTER TERMINAL"_lvl: {
		ret.inputs.resize(1);
		ret.i_output.reshape(30, 18);
	} break;
	case "BACK-REFERENCE REIFIER"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "DYNAMIC PATTERN DETECTOR"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "SEQUENCE GAP INTERPOLATOR"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "DECIMAL TO OCTAL CONVERTER"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "PROLONGED SEQUENCE SORTER"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "PRIME FACTOR CALCULATOR"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "SIGNAL EXPONENTIATOR"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "T20 NODE EMULATOR"_lvl: {
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
	} break;
	case "T31 NODE EMULATOR"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "WAVE COLLAPSE SUPERVISOR"_lvl: {
		ret.inputs.resize(4);
		ret.n_outputs.resize(1);
	} break;
	case "UNKNOWN"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(2);
	} break;
	default:
		throw std::invalid_argument{""};
	}
	return ret;
}
