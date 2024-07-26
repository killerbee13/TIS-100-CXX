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
#include "tis_random.hpp"
#include <kblib/random.h>

static_assert(xorshift128_engine(400).next_int(-10, 0) < 0);

static std::vector<word_t> make_random_array(std::uint32_t seed,
                                             std::uint32_t size, word_t min,
                                             word_t max) {
	xorshift128_engine engine(seed);
	std::vector<word_t> array(size);
	std::uint32_t num = 0;
	while (num < size) {
		array[num] = static_cast<word_t>(engine.next_int(min, max));
		num++;
	}
	return array;
}
static std::vector<word_t> make_random_array(xorshift128_engine& engine,
                                             std::uint32_t size, word_t min,
                                             word_t max) {
	std::vector<word_t> array(size);
	std::uint32_t num = 0;
	while (num < size) {
		array[num] = engine.next_int(min, max);
		num++;
	}
	return array;
}

static std::vector<word_t> make_composite_array(uint seed, word_t size,
                                                word_t sublistmin,
                                                word_t sublistmax,
                                                word_t valuemin,
                                                word_t valuemax) {
	xorshift128_engine engine(seed);
	std::vector<word_t> list;
	while (std::cmp_less(list.size(), size)) {
		int sublistsize = engine.next_int(sublistmin, sublistmax);
		int i = 0;
		for (; i < sublistsize; ++i) {
			list.push_back(engine.next_int(valuemin, valuemax));
		}
		list.push_back(0);
	}
	if (std::cmp_greater(list.size(), size)) {
		list.erase(list.begin() + size, list.end());
	}
	list.back() = 0;
	return list;
}
static std::vector<word_t> make_composite_array(xorshift128_engine& engine,
                                                word_t size, word_t sublistmin,
                                                word_t sublistmax,
                                                word_t valuemin,
                                                word_t valuemax) {
	std::vector<word_t> list;
	while (std::cmp_less(list.size(), size)) {
		int sublistsize = engine.next_int(sublistmin, sublistmax);
		int i = 0;
		for (; i < sublistsize; ++i) {
			list.push_back(engine.next_int(valuemin, valuemax));
		}
		list.push_back(0);
	}
	if (std::cmp_greater(list.size(), size)) {
		list.erase(list.begin() + size, list.end());
	}
	list.back() = 0;
	return list;
}

constexpr static word_t image_width = 30;
constexpr static word_t image_height = 18;
constexpr static int max_test_length = 39;

// use placeholder value 1 for sandbox levels
constexpr static std::array<std::uint32_t, layouts.size()> seeds{
    50,      2,       3,       4,       22,      //
    5,       9,       7,       19,      1,       //
    888,     18,      10,      6,       1,       //
    13,      14,      60,      15,      1,       //
    55,      16,      11,      12,      21,      //
    23,                                          //
    0 * 23,  1 * 23,  2 * 23,  3 * 23,  4 * 23,  //
    5 * 23,  6 * 23,  7 * 23,  8 * 23,  9 * 23,  //
    10 * 23, 11 * 23, 12 * 23, 13 * 23, 14 * 23, //
    15 * 23, 16 * 23, 17 * 23, 18 * 23, 19 * 23, //
    20 * 23, 21 * 23, 22 * 23, 23 * 23, 24 * 23, //
};

std::array<single_test, 3> static_suite(int id) {
	return {
	    random_test(id, seeds.at(static_cast<std::size_t>(id)) * 100),
	    random_test(id, seeds[static_cast<std::size_t>(id)] * 100 + 1),
	    random_test(id, seeds[static_cast<std::size_t>(id)] * 100 + 2),
	};
}

template <typename F>
void for_each_subsequence_of(std::vector<word_t>& in, word_t delim, F f) {
	auto start = in.begin();
	auto cur = start;
	for (; cur != in.end(); ++cur) {
		if (*cur == delim) {
			f(start, cur);
			start = cur + 1;
		}
	}
}

single_test random_test(int id, uint32_t seed) {
	// log_info("random_test(", id, ", ", seed, ")");
	single_test ret{};
	switch (id) {
	case "SELF-TEST DIAGNOSTIC"_lvl: {
		ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
		ret.inputs.push_back(
		    make_random_array(seed + 1, max_test_length, 10, 100));
		ret.n_outputs = ret.inputs;
	} break;
	case "SIGNAL AMPLIFIER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
		ret.n_outputs.resize(1);
		std::ranges::transform(ret.inputs[0],
		                       std::back_inserter(ret.n_outputs[0]),
		                       [](word_t x) { return 2 * x; });
	} break;
	case "DIFFERENTIAL CONVERTER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
		ret.inputs.push_back(
		    make_random_array(seed + 1, max_test_length, 10, 100));
		ret.n_outputs.resize(2);
		std::ranges::transform(ret.inputs[0], ret.inputs[1],
		                       std::back_inserter(ret.n_outputs[0]),
		                       [](word_t x, word_t y) { return x - y; });
		std::ranges::transform(ret.inputs[0], ret.inputs[1],
		                       std::back_inserter(ret.n_outputs[1]),
		                       [](word_t x, word_t y) { return y - x; });
	} break;
	case "SIGNAL COMPARATOR"_lvl: {
		ret.inputs.push_back(make_random_array(seed, max_test_length, -2, 3));
		ret.n_outputs.resize(3, std::vector<word_t>(max_test_length));
		for (auto [x, i] : kblib::enumerate(ret.inputs[0])) {
			ret.n_outputs[0][i] = (x > 0);
			ret.n_outputs[1][i] = (x == 0);
			ret.n_outputs[2][i] = (x < 0);
		}
	} break;
	case "SIGNAL MULTIPLEXER"_lvl: {
		ret.inputs.resize(3);
		ret.n_outputs.resize(1, std::vector<word_t>(max_test_length));
		ret.inputs[0] = make_random_array(seed, max_test_length, -30, 1);
		ret.inputs[1] = make_random_array(seed + 2, max_test_length, -1, 2);
		ret.inputs[2] = make_random_array(seed + 1, max_test_length, 0, 31);
		for (auto [x, i] : kblib::enumerate(ret.inputs[1])) {
			if (x <= 0) {
				ret.n_outputs[0][i] += ret.inputs[0][i];
			}
			if (x >= 0) {
				ret.n_outputs[0][i] += ret.inputs[2][i];
			}
		}
	} break;
	case "SEQUENCE GENERATOR"_lvl: {
		ret.inputs.push_back(make_random_array(seed, 13, 10, 100));
		ret.inputs.push_back(make_random_array(seed + 1, 13, 10, 100));
		ret.n_outputs.resize(1);
		for (const auto i : kblib::range(13u)) {
			auto v = std::minmax(ret.inputs[0][i], ret.inputs[1][i]);
			ret.n_outputs[0].push_back(v.first);
			ret.n_outputs[0].push_back(v.second);
			ret.n_outputs[0].push_back(0);
		}
	} break;
	case "SEQUENCE COUNTER"_lvl: {
		ret.inputs.push_back(
		    make_composite_array(seed, max_test_length, 0, 6, 10, 100));

		word_t sum{};
		word_t count{};
		ret.n_outputs.resize(2);
		for (auto w : ret.inputs[0]) {
			if (w == 0) {
				ret.n_outputs[0].push_back(std::exchange(sum, 0));
				ret.n_outputs[1].push_back(std::exchange(count, 0));
			} else {
				++count;
				sum = sat_add(sum, w);
			}
		}
	} break;
	case "SIGNAL EDGE DETECTOR"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(std::vector<word_t>(max_test_length));
		ret.inputs[0][1] = engine.next_int(25, 75);
		std::size_t i = 2;
		while (i < max_test_length) {
			switch (engine.next(0, 6)) {
			case 1:
				ret.inputs[0][i] = ret.inputs[0][i - 1] + engine.next_int(-11, -8);
				break;
			case 2:
				ret.inputs[0][i] = ret.inputs[0][i - 1] + engine.next_int(9, 12);
				break;
			default:
				ret.inputs[0][i] = ret.inputs[0][i - 1] + engine.next_int(-4, 5);
			}
			++i;
		}

		ret.n_outputs.resize(1);
		word_t prev = 0;
		for (auto w : ret.inputs[0]) {
			ret.n_outputs[0].push_back(std::abs(w - std::exchange(prev, w)) >= 10);
		}
	} break;
	case "INTERRUPT HANDLER"_lvl: {
		ret.inputs.resize(4, std::vector<word_t>(1));
		ret.n_outputs.resize(1, std::vector<word_t>(1));
		std::array<bool, 4> array2{};

		xorshift128_engine engine(seed);
		for (std::size_t m = 1; m < max_test_length; ++m) {
			auto rand = engine.next(0, 6);
			if (rand < 4) {
				array2[rand] = not array2[rand];
				ret.n_outputs[0].push_back(
				    array2[rand] ? static_cast<word_t>(rand + 1) : 0);
			} else {
				ret.n_outputs[0].push_back(0);
			}
			for (std::size_t n = 0; n < 4; ++n) {
				ret.inputs[n].push_back(array2[n]);
			}
		}
	} break;
	case "SIMPLE SANDBOX"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "SIGNAL PATTERN DETECTOR"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(make_random_array(engine, max_test_length, 0, 6));
		for (std::size_t i = 0; i < 8; ++i) {
			std::size_t num = engine.next(0, 36);
			ret.inputs[0][num] = 0;
			ret.inputs[0][num + 1] = 0;
			ret.inputs[0][num + 2] = 0;
			num = engine.next(0, 35);
			ret.inputs[0][num] = engine.next_int(1, 6);
			ret.inputs[0][num + 1] = 0;
			ret.inputs[0][num + 2] = 0;
			ret.inputs[0][num + 3] = engine.next_int(1, 6);
		}
		ret.n_outputs.push_back(std::vector<word_t>(max_test_length));
		for (std::size_t j = 0; j < max_test_length; ++j) {
			ret.n_outputs[0][j]
			    = (j > 1 and ret.inputs[0][j - 2] == 0
			       and ret.inputs[0][j - 1] == 0 and ret.inputs[0][j] == 0);
		}
	} break;
	case "SEQUENCE PEAK DETECTOR"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(
		    make_composite_array(engine, max_test_length, 3, 6, 10, 100));
		ret.inputs[0][37] = engine.next_int(10, 100);
		ret.inputs[0].back() = 0;
		ret.n_outputs.resize(2);

		for_each_subsequence_of(ret.inputs[0], 0, [&](auto begin, auto end) {
			auto v = std::ranges::minmax_element(begin, end);
			ret.n_outputs[0].push_back(*v.min);
			ret.n_outputs[1].push_back(*v.max);
		});
	} break;
	case "SEQUENCE REVERSER"_lvl: {
		ret.inputs.push_back(
		    make_composite_array(seed, max_test_length, 0, 6, 10, 100));
		ret.n_outputs.resize(1);

		for_each_subsequence_of(ret.inputs[0], 0, [&](auto begin, auto end) {
			std::ranges::reverse_copy(begin, end,
			                          std::back_inserter(ret.n_outputs[0]));
			ret.n_outputs[0].push_back(0);
		});
	} break;
	case "SIGNAL MULTIPLIER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, max_test_length, 0, 10));
		ret.inputs.push_back(make_random_array(seed + 1, max_test_length, 0, 10));
		ret.n_outputs.push_back(std::vector<word_t>(max_test_length));
		std::ranges::transform(ret.inputs[0], ret.inputs[1],
		                       ret.n_outputs[0].begin(), std::multiplies<>{});
	} break;
	case "STACK MEMORY SANDBOX"_lvl: {
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
	} break;
	case "IMAGE TEST PATTERN 1"_lvl: {
		ret.i_output.reshape(image_width, image_height,
		                     tis_pixel{tis_pixel::color::C_white});
	} break;
	case "IMAGE TEST PATTERN 2"_lvl: {
		ret.i_output = checkerboard(image_width, image_height);
	} break;
	case "EXPOSURE MASK VIEWER"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.resize(1);
		ret.i_output.reshape(image_width, image_height);
		for (int i = 0; i < 9; ++i) {
			word_t num2{};
			word_t num3{};
			word_t num4{};
			word_t num5{};
			{
			IL_0030:
				num2 = engine.next_int(3, 6);
				num3 = engine.next_int(3, 6);
				num4 = engine.next_int(1, image_width - 1 - num2);
				num5 = engine.next_int(1, image_height - 1 - num3);
				for (int j = -1; j < num2 + 1; ++j) {
					for (int k = -1; k < num3 + 1; ++k) {
						std::size_t index = static_cast<std::size_t>(
						    num4 + j + (num5 + k) * image_width);
						if (ret.i_output.at(index) != 0) {
							goto IL_0030;
						}
					}
				}
			}
			ret.inputs[0].push_back(num4);
			ret.inputs[0].push_back(num5);
			ret.inputs[0].push_back(num2);
			ret.inputs[0].push_back(num3);
			for (int l = 0; l < num2; ++l) {
				for (int m = 0; m < num3; ++m) {
					std::size_t index = static_cast<std::size_t>(
					    num4 + l + (num5 + m) * image_width);
					ret.i_output.at(index) = 3;
				}
			}
		}
	} break;
	case "HISTOGRAM VIEWER"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(std::vector<word_t>(image_width));
		ret.i_output.reshape(image_width, image_height);
		ret.inputs[0][0] = engine.next_int(3, 14);
		for (std::size_t x = 1; x < image_width; ++x) {
			if (engine.next(0, 4) != 0) {
				ret.inputs[0][x]
				    = std::clamp(static_cast<word_t>(ret.inputs[0][x - 1]
				                                     + engine.next_int(-2, 3)),
				                 word_t{1}, word_t{image_height - 1});
			} else {
				ret.inputs[0][x] = engine.next_int(3, 14);
			}
		}
		for (int y = 0; y < image_height; ++y) {
			for (int x = 0; x < image_width; ++x) {
				ret.i_output.at(x, y)
				    = (kblib::to_signed(image_height - y)
				       <= ret.inputs[0][static_cast<std::size_t>(x)])
				          ? 3
				          : 0;
			}
		}
	} break;
	case "IMAGE CONSOLE SANDBOX"_lvl: {
		ret.inputs.resize(1);
		ret.i_output.reshape(36, 22);
	} break;
	case "SIGNAL WINDOW FILTER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
		ret.n_outputs.resize(2);
		for (std::size_t idx = 0; idx < max_test_length; ++idx) {
			word_t t = ret.inputs[0][idx]
			           + ((idx >= 1) ? ret.inputs[0][idx - 1] : 0)
			           + ((idx >= 2) ? ret.inputs[0][idx - 2] : 0);
			ret.n_outputs[0].push_back(t);
			t += ((idx >= 3) ? ret.inputs[0][idx - 3] : 0)
			     + ((idx >= 4) ? ret.inputs[0][idx - 4] : 0);
			ret.n_outputs[1].push_back(t);
		}
	} break;
	case "SIGNAL DIVIDER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, 39, 10, 100));
		ret.inputs.push_back(make_random_array(seed + 1, 39, 1, 10));
		ret.n_outputs.resize(2, std::vector<word_t>(max_test_length));
		for (std::size_t i = 0; i < max_test_length; ++i) {
			ret.n_outputs[0][i] = ret.inputs[0][i] / ret.inputs[1][i];
			ret.n_outputs[1][i] = ret.inputs[0][i] % ret.inputs[1][i];
		}
	} break;
	case "SEQUENCE INDEXER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, 10, 100, 1000));
		ret.inputs[0].push_back(0);
		ret.inputs.push_back(make_random_array(seed, max_test_length, 0, 10));
		ret.n_outputs.resize(1, std::vector<word_t>(max_test_length));
		for (std::size_t i = 0; i < max_test_length; ++i) {
			ret.n_outputs[0][i]
			    = ret.inputs[0][kblib::to_unsigned(ret.inputs[1][i])];
		}
	} break;
	case "SEQUENCE SORTER"_lvl: {
		ret.inputs.push_back(
		    make_composite_array(seed, max_test_length, 4, 8, 10, 100));
		ret.n_outputs.resize(1);
		ret.n_outputs[0].reserve(max_test_length);

		std::vector<word_t> sublist;
		for_each_subsequence_of(ret.inputs[0], 0, [&](auto begin, auto end) {
			sublist.assign(begin, end);
			std::ranges::sort(sublist);
			ret.n_outputs[0].insert(ret.n_outputs[0].end(), sublist.begin(),
			                        sublist.end());
			ret.n_outputs[0].push_back(0);
		});
	} break;
	case "STORED IMAGE DECODER"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.resize(1);
		ret.i_output.reshape(image_width, image_height);
		std::vector<tis_pixel> image;
		while (image.size() < image_width * image_height) {
			word_t num = engine.next_int(20, 45);
			word_t num2 = engine.next_int(0, 4);
			ret.inputs[0].push_back(num);
			ret.inputs[0].push_back(num2);
			image.insert(image.end(), kblib::to_unsigned(num), tis_pixel(num2));
		}
		image.resize(ret.i_output.size());
		ret.i_output.assign(std::move(image));
	} break;
	case "UNKNOWN"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(std::vector<word_t>(max_test_length));
		ret.n_outputs.resize(2);
		while (ret.n_outputs[0].size() < max_test_length) {
			word_t item = engine.next_int(0, 4);
			word_t size = engine.next_int(2, 5);
			for (int i = 0; i < size; ++i) {
				if (ret.n_outputs[0].size() < max_test_length) {
					ret.n_outputs[0].push_back(item);
				}
			}
		}
		for (std::size_t j = 0; j < max_test_length; ++j) {
			ret.inputs[0][j]
			    = ret.n_outputs[0][j] * 25 + 12 + engine.next_int(-6, 7);
		}
		ret.n_outputs[0].back() = -1;
		ret.inputs[0].back() = -1;
		word_t num4 = -1;
		word_t num5 = 0;
		for (std::size_t k = 0; k < max_test_length; ++k) {
			if (num4 != ret.n_outputs[0][k]) {
				if (num4 >= 0) {
					ret.n_outputs[1].push_back(num5);
					ret.n_outputs[1].push_back(num4);
				}
				num4 = ret.n_outputs[0][k];
				num5 = 1;
			} else {
				++num5;
			}
		}
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
		ret.i_output.reshape(image_width, image_height);
	} break;
	case "CHARACTER TERMINAL"_lvl: {
		ret.inputs.resize(1);
		ret.i_output.reshape(image_width, image_height);
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
	default:
		throw std::invalid_argument{""};
	}
	return ret;
}
