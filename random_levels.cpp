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

#include "random_levels.hpp"
#include "builtin_levels.hpp"
#include "tis_random.hpp"
#include <kblib/random.h>

#include <ranges>

static_assert(xorshift128_engine(400).next_int(-10, 0) < 0);

static word_vec make_random_array(xorshift128_engine& engine,
                                  std::uint32_t size, word_t min, word_t max) {
	word_vec array(size);
	std::uint32_t num = 0;
	while (num < size) {
		array[num] = engine.next_int(min, max);
		num++;
	}
	return array;
}
static word_vec make_random_array(std::uint32_t seed, std::uint32_t size,
                                  word_t min, word_t max) {
	xorshift128_engine engine(seed);
	return make_random_array(engine, size, min, max);
}

static word_vec make_composite_array(xorshift128_engine& engine, word_t size,
                                     word_t sublistmin, word_t sublistmax,
                                     word_t valuemin, word_t valuemax) {
	word_vec list;
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
static word_vec make_composite_array(uint seed, word_t size, word_t sublistmin,
                                     word_t sublistmax, word_t valuemin,
                                     word_t valuemax) {
	xorshift128_engine engine(seed);
	return make_composite_array(engine, size, sublistmin, sublistmax, valuemin,
	                            valuemax);
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
	    *random_test(id, seeds.at(static_cast<std::size_t>(id)) * 100),
	    *random_test(id, seeds[static_cast<std::size_t>(id)] * 100 + 1),
	    *random_test(id, seeds[static_cast<std::size_t>(id)] * 100 + 2),
	};
}

template <typename F>
void for_each_subsequence_of(word_vec& in, word_t delim, F f) {
	auto start = in.begin();
	auto cur = start;
	for (; cur != in.end(); ++cur) {
		if (*cur == delim) {
			f(start, cur);
			start = cur + 1;
		}
	}
}

word_vec empty_vec(word_t size = max_test_length) {
	assert(size >= 0);
	return word_vec(kblib::to_unsigned(size));
}

std::optional<single_test> random_test(int id, uint32_t seed) {
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
		ret.n_outputs.resize(3, empty_vec());
		for (auto [x, i] : kblib::enumerate(ret.inputs[0])) {
			ret.n_outputs[0][i] = (x > 0);
			ret.n_outputs[1][i] = (x == 0);
			ret.n_outputs[2][i] = (x < 0);
		}
	} break;
	case "SIGNAL MULTIPLEXER"_lvl: {
		ret.inputs.resize(3);
		ret.n_outputs.resize(1, empty_vec());
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
		xorshift128_engine engine(seed + 1);
		ret.inputs.push_back(make_random_array(engine, 13, 10, 100));
		uint idx = engine.next(0, 13);
		ret.inputs[0][idx] = ret.inputs[1][idx] = engine.next_int(10, 100);
		ret.n_outputs.resize(1);
		for (const auto i : range(13u)) {
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
		ret.inputs.push_back(empty_vec());
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
		ret.inputs.resize(4, empty_vec(1));
		ret.n_outputs.resize(1, empty_vec(1));
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
		ret.n_outputs.push_back(empty_vec());
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
		ret.n_outputs.push_back(empty_vec());
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
			word_t w{};
			word_t h{};
			word_t x_c{};
			word_t y_c{};
			std::size_t iterations{};
			{
			retry:
				// This code sometimes places 8 rectangles in such a way that there
				// is no valid position for a 9th, and gets stuck in an infinite
				// loop. 99th percentile of iterations required to place the 9th
				// rectangle is 217, so using 250 will cause fewer than 1% of seeds
				// to be skipped. This check has some false positives, some seeds
				// may find a place to fit the last rectangle on the 251st
				// iteration, but a proper largest_contiguous_rectangle check would
				// be slower and need more code.
				if (iterations > 250) {
					log_debug("skipped placing rectangle ", i);
					return std::nullopt;
				}
				w = engine.next_int(3, 6);
				h = engine.next_int(3, 6);
				x_c = engine.next_int(1, image_width - 1 - w);
				y_c = engine.next_int(1, image_height - 1 - h);
				// Check if the rectangle would overlap or touch any already placed
				// rectangle
				for (int j = -1; j < w + 1; ++j) {
					for (int k = -1; k < h + 1; ++k) {
						if (ret.i_output.at(x_c + j, y_c + k) != 0) {
							++iterations;
							goto retry;
						}
					}
				}
			}
#if HISTOGRAM
			histogram[i].push_back(iterations);
#endif
			ret.inputs[0].push_back(x_c);
			ret.inputs[0].push_back(y_c);
			ret.inputs[0].push_back(w);
			ret.inputs[0].push_back(h);
			for (int j = 0; j < w; ++j) {
				for (int k = 0; k < h; ++k) {
					[[maybe_unused]] std::size_t index = static_cast<std::size_t>(
					    x_c + j + (y_c + k) * image_width);
					ret.i_output.at(x_c + j, y_c + k) = 3;
					assert(ret.i_output.at(index)
					       == ret.i_output.at(x_c + j, y_c + k));
				}
			}
			log_debug_r([&] { return "image:\n" + ret.i_output.write_text(); });
		}
	} break;
	case "HISTOGRAM VIEWER"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(empty_vec(image_width));
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
				    = (to_signed(image_height - y)
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
			word_t t = ret.inputs[0][idx];
			t += ((idx >= 1) ? ret.inputs[0][idx - 1] : word_t{});
			t += ((idx >= 2) ? ret.inputs[0][idx - 2] : word_t{});
			ret.n_outputs[0].push_back(t);
			t += ((idx >= 3) ? ret.inputs[0][idx - 3] : word_t{});
			t += ((idx >= 4) ? ret.inputs[0][idx - 4] : word_t{});
			ret.n_outputs[1].push_back(t);
		}
	} break;
	case "SIGNAL DIVIDER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, 39, 10, 100));
		ret.inputs.push_back(make_random_array(seed + 1, 39, 1, 10));
		ret.n_outputs.resize(2, empty_vec());
		for (std::size_t i = 0; i < max_test_length; ++i) {
			ret.n_outputs[0][i]
			    = static_cast<word_t>(ret.inputs[0][i] / ret.inputs[1][i]);
			ret.n_outputs[1][i]
			    = static_cast<word_t>(ret.inputs[0][i] % ret.inputs[1][i]);
		}
	} break;
	case "SEQUENCE INDEXER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, 10, 100, 1000));
		ret.inputs[0].push_back(0);
		ret.inputs.push_back(make_random_array(seed, max_test_length, 0, 10));
		ret.n_outputs.resize(1, empty_vec());
		for (std::size_t i = 0; i < max_test_length; ++i) {
			ret.n_outputs[0][i] = ret.inputs[0][to_unsigned(ret.inputs[1][i])];
		}
	} break;
	case "SEQUENCE SORTER"_lvl: {
		ret.inputs.push_back(
		    make_composite_array(seed, max_test_length, 4, 8, 10, 100));
		ret.n_outputs.resize(1);
		ret.n_outputs[0].reserve(max_test_length);

		word_vec sublist;
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
			image.insert(image.end(), to_unsigned(num), tis_pixel(num2));
		}
		image.resize(ret.i_output.size());
		ret.i_output.assign(std::move(image));
	} break;
	case "UNKNOWN"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(empty_vec());
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
			ret.inputs[0][j] = static_cast<word_t>(ret.n_outputs[0][j] * 25 + 12
			                                       + engine.next_int(-6, 7));
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
		lua_random engine(kblib::to_signed(seed));
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
		word_vec& out = ret.n_outputs[0];
		bool prevempty = true;
		bool canzero = true;
		do {
			std::size_t maxmax;
			if (out.size() == 26) {
				maxmax = 10;
			} else if (out.size() < 28) {
				maxmax = 11;
			} else {
				maxmax = 38 - out.size();
			}

			std::size_t maxout;
			if (maxmax < 10) {
				maxout = maxmax;
			} else {
				do {
					maxout = static_cast<std::size_t>(
					    engine.next(0, static_cast<word_t>(maxmax)));
				} while (! canzero and maxout == 0);
			}

			std::size_t count1;
			if (prevempty and maxout >= 2) {
				count1 = static_cast<std::size_t>(
				    engine.next(1, static_cast<word_t>(maxout - 1)));
			} else {
				count1 = static_cast<std::size_t>(
				    engine.next(0, static_cast<word_t>(maxout)));
			}
			if (maxout == 0) {
				canzero = false;
			}

			prevempty = (count1 == 0 or count1 == maxout);
			word_vec outseq(maxout);
			word_vec in1seq(count1);
			word_vec in2seq(maxout - count1);
			if (maxout > 0) {
				for (std::size_t i = 0; i < maxout; i++) {
					word_t val;
					while (true) {
						val = engine.next(10, 99);
						if (std::find(outseq.begin(), outseq.end(), val)
						    == outseq.end()) {
							break;
						}
					}
					outseq[i] = val;
					if (i < count1) {
						in1seq[i] = val;
					} else {
						in2seq[i - count1] = val;
					}
				}
			}
			std::ranges::sort(outseq.begin(), outseq.end());
			std::ranges::sort(in1seq.begin(), in1seq.end());
			std::ranges::sort(in2seq.begin(), in2seq.end());
			for (std::size_t i = 0; i < maxout; i++) {
				out.push_back(outseq[i]);
				if (i < count1) {
					ret.inputs[0].push_back(in1seq[i]);
				} else {
					ret.inputs[1].push_back(in2seq[i - count1]);
				}
			}
			out.push_back(0);
			ret.inputs[0].push_back(0);
			ret.inputs[1].push_back(0);
		} while (out.size() < max_test_length);
	} break;
	case "INTEGER SERIES CALCULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
		for (std::size_t i = 0; i < max_test_length; i++) {
			word_t n = engine.next(1, 44);
			ret.inputs[0].push_back(n);
			ret.n_outputs[0].push_back(static_cast<word_t>(n * (n + 1) / 2));
		}
	} break;
	case "SEQUENCE RANGE LIMITER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(3);
		ret.n_outputs.resize(1);
		word_vec& input = ret.inputs[1];
		word_vec& mininput = ret.inputs[0];
		word_vec& maxinput = ret.inputs[2];
		word_vec& output = ret.n_outputs[0];
		// make minimums small but not too small
		for (std::size_t i = 0; i < 6; i++) {
			mininput.push_back(engine.next(3, 9) * 5);
		}
		// make maximums big
		for (std::size_t i = 0; i < 6; i++) {
			maxinput.push_back(engine.next(10, 17) * 5);
		}
		// For now, just make five sequences that are five long (with a
		// sixth value being a 0 terminator)
		// Manually calculate correct output values
		for (std::size_t i = 0; i < 6; i++) {
			for (std::size_t j = 0; j < 5; j++) {
				word_t val = engine.next(10, 99);
				input.push_back(val);
				output.push_back(std::clamp(val, mininput[i], maxinput[i]));
			}
			input.push_back(0);
			output.push_back(0);
		}
	} break;
	case "SIGNAL ERROR CORRECTOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2);
		ret.n_outputs.resize(2);
		word_vec& in_a = ret.inputs[0];
		word_vec& in_b = ret.inputs[1];
		word_vec& out_a = ret.n_outputs[0];
		word_vec& out_b = ret.n_outputs[1];
		for (int i = 0; i < max_test_length; i++) {
			word_t r = engine.next(1, 4);
			word_t a = engine.next(10, 99);
			word_t b = engine.next(10, 99);
			if (r == 1) {
				in_a.push_back(-1);
				in_b.push_back(b);
				out_a.push_back(b);
				out_b.push_back(b);
			} else if (r == 2) {
				in_a.push_back(a);
				in_b.push_back(-1);
				out_a.push_back(a);
				out_b.push_back(a);
			} else {
				in_a.push_back(a);
				in_b.push_back(b);
				out_a.push_back(a);
				out_b.push_back(b);
			}
		}
	} break;
	case "SUBSEQUENCE EXTRACTOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
		word_vec& in_indexes = ret.inputs[0];
		word_vec& in_seq = ret.inputs[1];
		word_vec& out = ret.n_outputs[0];

		std::array<word_t, 8> seq_lengths = {2, 3, 3, 4, 4, 4, 5, 6};
		// Shuffle the subsequence lengths:
		for (std::size_t i = seq_lengths.size() - 1; i >= 1; i--) {
			std::size_t j
			    = static_cast<std::size_t>(engine.next(0, static_cast<word_t>(i)));
			std::swap(seq_lengths[i], seq_lengths[j]);
		}

		for (word_t len : seq_lengths) {
			// Generate input sequences:
			for (word_t j = 0; j < len; j++) {
				in_seq.push_back(engine.next(10, 99));
			}
			in_seq.push_back(0);
			// Generate subsequence indexes:
			word_t sublen = engine.next(2, len);
			word_t first = engine.next(0, len - sublen);
			word_t last = static_cast<word_t>(first + sublen - 1);
			in_indexes.push_back(first);
			in_indexes.push_back(last);
			// Generate correct output:
			auto in_it = in_seq.end() - len - 1 + first;
			out.insert(out.end(), in_it, in_it + sublen);
			out.push_back(0);
		}
	} break;
	case "SIGNAL PRESCALER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(3);
		for (int i = 0; i < max_test_length; i++) {
			word_t val = engine.next(1, 120);
			ret.n_outputs[2].push_back(val);
			ret.n_outputs[1].push_back(val * 2);
			ret.n_outputs[0].push_back(val * 4);
			ret.inputs[0].push_back(val * 8);
		}
	} break;
	case "SIGNAL AVERAGER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
		for (int i = 0; i < max_test_length; i++) {
			word_t valA = engine.next(100, 999);
			word_t valB = engine.next(100, 999);
			ret.inputs[0].push_back(valA);
			ret.inputs[1].push_back(valB);
			ret.n_outputs[0].push_back(static_cast<word_t>((valA + valB) / 2));
		}
	} break;
	case "SUBMAXIMUM SELECTOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(4);
		ret.n_outputs.resize(1);
		for (int i = 0; i < max_test_length; i++) {
			std::array<word_t, 4> group;
			for (std::size_t j = 0; j < 4; j++) {
				word_t v = engine.next(0, 99);
				group[j] = v;
				ret.inputs[j].push_back(v);
			}

			std::nth_element(group.begin(), group.begin() + 2, group.end());
			ret.n_outputs[0].push_back(group[2]);
		}
	} break;
	case "DECIMAL DECOMPOSER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(3);
		for (int i = 0; i < max_test_length; i++) {
			word_t digits = engine.next(0, 2);
			word_t val;
			if (digits == 0) {
				val = engine.next(0, 9);
			} else if (digits == 1) {
				val = engine.next(10, 99);
			} else {
				val = engine.next(100, 999);
			}
			ret.inputs[0].push_back(val);
			ret.n_outputs[0].push_back(val / 100);
			ret.n_outputs[1].push_back((val % 100) / 10);
			ret.n_outputs[2].push_back(val % 10);
		}
	} break;
	case "SEQUENCE MODE CALCULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);

		// generate input stream
		int last_zero = -1;
		for (int i = 0; i < max_test_length - 1; i++) {
			ret.inputs[0].push_back(engine.next(1, 5));
			// tuned to give nice balance of sequence lengths
			if (i - last_zero > 3 and engine.next_double() < 0.5
			    and i < max_test_length - 2) {
				ret.inputs[0].back() = 0;
				last_zero = i;
			}
		}
		ret.inputs[0].push_back(0);

		// generate output stream
		word_vec sequence = {};
		for (word_t input : ret.inputs[0]) {
			if (input == 0) {
				// generate frequency map
				std::array frequency = {0u, 0u, 0u, 0u, 0u};
				for (word_t element : sequence) {
					frequency[static_cast<std::size_t>(element - 1)]++;
				}

				// determine mode, and whether it is unique
				uint max_frequency = 0;
				uint most_frequent = 1;
				bool unique = true;
				for (uint k = 0; k < 5; k++) {
					if (frequency[k] > max_frequency) {
						unique = true;
						most_frequent = k + 1;
						max_frequency = frequency[k];
					} else if (frequency[k] == max_frequency) {
						unique = false;
					}
				}

				if (unique) {
					ret.n_outputs[0].push_back(static_cast<word_t>(most_frequent));
				} else {
					ret.n_outputs[0].push_back(0);
				}

				sequence.clear();
			} else {
				sequence.push_back(input);
			}
		}
	} break;
	case "SEQUENCE NORMALIZER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
		// Current sequence from 'input'
		word_vec cur_seq = {};

		for (int i = 0; i < max_test_length - 1; i++) {
			word_t val = engine.next(1, 99);
			ret.inputs[0].push_back(val);
			cur_seq.push_back(val);

			// Possibly end sequence ensuring that the it is at least of length 3
			// and no longer than 8.
			// Also ensure final sequence is ended properly - may cause shorter
			// sequence length.
			if ((engine.next(1, 3) % 3 == 0 and cur_seq.size() > 2)
			    or (cur_seq.size() > 7) or (i == max_test_length - 3)) {
				// Generate 'output'
				word_t min_in_seq = *std::ranges::min_element(cur_seq);
				for (word_t seqval : cur_seq) {
					ret.n_outputs[0].push_back(seqval - min_in_seq);
				}

				// Add the sequence terminating -1
				i++;
				ret.inputs[0].push_back(-1);
				ret.n_outputs[0].push_back(-1);

				cur_seq.clear();
			}
		}
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
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.i_output.reshape(image_width, image_height);

		// Initialize the image
		std::vector<tis_pixel> image(image_width * image_height, tis_pixel{0});

		// Helper methods
		auto shuffle = [&engine](std::vector<word_t> t) {
			for (std::size_t n = t.size() - 1; n > 0; n--) {
				// n is now the last pertinent index
				std::size_t k = engine.next(0, static_cast<word_t>(n));
				// Quick swap
				std::swap(t[n], t[k]);
			}
			return t;
		};

		auto one_to_n = [](word_t n) {
			std::vector<word_t> a = {};
			for (word_t i = 0; i < n; i++) {
				a.push_back(i + 1);
			}
			return a;
		};

		auto makePoints = [&](std::size_t n, word_t maxX, word_t maxY) {
			std::vector<word_t> xCoors = {0};
			std::vector<word_t> yCoors = {0};
			std::vector<word_t> remainingX = shuffle(one_to_n(maxX));
			std::vector<word_t> remainingY = shuffle(one_to_n(maxY));
			while (xCoors.size() < n) {
				for (std::size_t i = 0; i < remainingX.size(); i++) {
					int dx = std::abs(xCoors.back() - remainingX[i]);
					if (dx >= 3 and dx <= 14) {
						xCoors.push_back(remainingX[i]);
						remainingX.erase(remainingX.begin() + to_signed(i));
						break;
					}
				}
				for (std::size_t i = 0; i < remainingY.size(); i++) {
					int dy = std::abs(yCoors.back() - remainingY[i]);
					if (dy >= 3 and dy <= 14) {
						yCoors.push_back(remainingY[i]);
						remainingY.erase(remainingY.begin() + to_signed(i));
						break;
					}
				}
			}
			return std::views::zip(xCoors, yCoors)
			       | std::ranges::to<std::vector>();
		};

		// Construct set of 11 points where
		//		no two points share an X or Y coordinate, and
		//		no adjacent points have X or Y coordinates less than 4 or more than
		// 15 apart.
		auto points = makePoints(11, image_width - 1, image_height - 1);

		// Draw lines, alternating from horizontal to vertical, between each
		// adjacent pair of points.
		for (std::size_t i = 1; i < points.size(); i++) {
			auto [xOne, yOne] = points[i - 1];
			auto [xTwo, yTwo] = points[i];

			word_t dx;
			if (xTwo < xOne) {
				ret.inputs[0].push_back(180);
				dx = -1;
			} else {
				ret.inputs[0].push_back(0);
				dx = 1;
			}
			for (word_t x = xOne; x != xTwo + dx; x += dx) {
				image[x + yOne * image_width] = tis_pixel{3};
			}
			ret.inputs[0].push_back(
			    static_cast<word_t>(std::abs(xOne - xTwo) + 1));

			// Exit early if the path is already long enough.
			if (ret.inputs[0].size() == max_test_length - 1) {
				break;
			}

			word_t dy;
			if (yTwo < yOne) {
				ret.inputs[0].push_back(90);
				dy = -1;
			} else {
				ret.inputs[0].push_back(270);
				dy = 1;
			}
			for (word_t y = yOne; y != yTwo + dy; y += dy) {
				image[xTwo + y * image_width] = tis_pixel{3};
			}
			ret.inputs[0].push_back(
			    static_cast<word_t>(std::abs(yOne - yTwo) + 1));
		}
		ret.i_output.assign(std::move(image));
	} break;
	case "CHARACTER TERMINAL"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.i_output.reshape(image_width, image_height);

		std::vector<tis_pixel> image(image_width * image_height, tis_pixel{0});

		// replace 2d arrays for alternative characters
		bool char_decode[][2][2] = {{{0, 0}, {0, 0}},
		                            {{1, 1}, {0, 0}},
		                            {{1, 0}, {0, 1}},
		                            {{0, 1}, {1, 0}},
		                            {{1, 1}, {1, 0}}};

		// place the character c into the image at x y
		auto render_character = [&](word_t x, word_t y, word_t c) {
			auto ch = char_decode[c];
			for (int a : {0, 1}) {
				for (int b : {0, 1}) {
					if (ch[a][b]) {
						// color for the character is set here
						image[x + a + (y + b) * image_width] = tis_pixel{3};
					}
				}
			}
		};

		auto& input = ret.inputs[0];
		for (std::size_t i = 0; i < max_test_length; i++) {
			input.push_back(engine.next(1, 4));
		}
		input.push_back(0);

		// these values are choosen to allow a bit of undefined behavior; I do not
		// pick whether a 0 at after the 10th character should be an empty line.
		// The prerolled sample will not have that, so I avoid testing the user on
		// that case because it seemed unfair.  I think both ways could be valid.
		input[engine.next(12, 16)] = 0;
		input[engine.next(28, 31)] = 0;

		// render image
		word_t x = -1;
		word_t y = 0;
		for (std::size_t i = 0; i < max_test_length; i++) {
			if (input[i] == 0 or x == 9) {
				x = 0;
				y++;
			} else {
				x++;
			}
			render_character(x * 3, y * 3, input[i + 1]);
		}
		input.erase(input.begin());
		ret.i_output.assign(std::move(image));
	} break;
	case "BACK-REFERENCE REIFIER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
		auto& input_refs = ret.inputs[0];
		auto& input_values = ret.inputs[1];
		for (ssize_t i = 0; i < max_test_length; i++) {
			word_t ref = 0;
			if (engine.next(0, 1) == 0) {
				ref = engine.next(-4, -1);
				if (i + ref < 0) {
					ref = 0;
				}
			}
			input_values.push_back(engine.next(10, 99));
			input_refs.push_back(ref);
			ret.n_outputs[0].push_back(input_values[i + ref]);
		}
	} break;
	case "DYNAMIC PATTERN DETECTOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
		auto& pattern = ret.inputs[0];
		auto& input = ret.inputs[1];

		for (int i = 0; i < 12; i++) {
			// RNG manip (There has to be a 42 in the pattern.)
			engine.next(1, 101);
		}

		for (int i = 0; i < 3; i++) {
			pattern.push_back(engine.next(1, 42)); // PATTERN gen
		}
		pattern.push_back(0);
		for (int i = 0; i < max_test_length; i++) {
			input.push_back(engine.next(1, 42)); // SEQUENCE gen
		}

		for (int k = 0; k < 2; k++) {
			word_t j = engine.next(1, 37); // pattern 123 potential extra matches
			for (int i = 0; i < 3; i++) {
				input[i + j - 1] = pattern[i];
			}
		}
		for (int k = 0; k < 3; k++) {
			word_t j = engine.next(1, 37); // pattern 23 these may be overwriten
			for (int i = 1; i < 3; i++) {
				input[i + j - 1] = pattern[i];
			}
		}

		// The following are guararnteed to be complete ( do I need a 123123? )
		word_t j = engine.next(1, 7); // pattern 123
		for (int i = 0; i < 3; i++) {
			input[i + j - 1] = pattern[i];
		}

		// pattern 1223 Don't know if guaranteed X23 is also needed.
		j = engine.next(10, 13);
		for (int i = 0; i < 2; i++) {
			input[i + j - 1] = pattern[i];
		}
		for (int i = 1; i < 3; i++) {
			input[i + j] = pattern[i];
		}

		j = engine.next(17, 23); // pattern 1123
		input[j - 1] = pattern[0];
		for (int i = 0; i < 3; i++) {
			input[j + i] = pattern[i];
		}

		j = engine.next(27, 35); // pattern 12123
		input[j - 1] = pattern[0];
		input[j] = pattern[1];
		for (int i = 0; i < 3; i++) {
			input[j + i + 1] = pattern[i];
		}

		ret.n_outputs[0].push_back(0);
		ret.n_outputs[0].push_back(0);
		for (int i = 2; i < max_test_length; i++) {
			if (input[i - 2] == pattern[0] and input[i - 1] == pattern[1]
			    and input[i] == pattern[2]) {
				ret.n_outputs[0].push_back(1);
			} else {
				ret.n_outputs[0].push_back(0);
			}
		}
	} break;
	case "SEQUENCE GAP INTERPOLATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);

		auto shuffle_tables = [&](word_vec& t) {
			for (auto i : kblib::range(std::ssize(t) - 1, 0z, -1)) {
				auto j = engine.next(0, static_cast<word_t>(i));
				std::swap(t[i], t[j]);
			}
		};

		std::array lengths{5, 4, 4, 4, 5, 4, 5, 4, 4};
		for (auto length : lengths) {
			word_t min = engine.next(10, 90);
			word_t max = static_cast<word_t>(min + length - 1);
			auto missing_value = engine.next(min + 1, max - 1);
			word_vec values;
			for (auto i : kblib::range<word_t>(min, max + 1)) {
				if (i != missing_value) {
					values.push_back(i);
				}
			}
			shuffle_tables(values);
			ret.inputs[0].insert(ret.inputs[0].end(), values.begin(),
			                     values.end());
			ret.inputs[0].push_back(0);
			ret.n_outputs[0].push_back(missing_value);
		}
	} break;
	case "DECIMAL TO OCTAL CONVERTER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
		auto to_octal = [](word_t i) { return (i / 8) * 10 + (i % 8); };

		for ([[maybe_unused]] auto _ : kblib::range(max_test_length)) {
			auto i = ret.inputs[0].emplace_back(engine.next(1, 63));
			ret.n_outputs[0].push_back(static_cast<word_t>(to_octal(i)));
		}
	} break;
	case "PROLONGED SEQUENCE SORTER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		std::array<int, 10> counts{};
		int zeros = 10;
		for (std::size_t i = 0; i < max_test_length - 1; ++i) {
			do {
				ret.inputs[0][i] = engine.next(0, 9);
				ret.n_outputs[0][i] = ret.inputs[0][i];
			} while (not (
			    zeros > 1
			    or (zeros == 1 and counts[to_unsigned(ret.inputs[0][i])] > 0)));
			if (counts[to_unsigned(ret.inputs[0][i])] == 0) {
				--zeros;
			}
			++counts[to_unsigned(ret.inputs[0][i])];
		}
		ret.inputs[0].back() = -1;
		std::ranges::sort(ret.n_outputs[0].begin(), ret.n_outputs[0].end() - 1);
		ret.n_outputs[0].back() = -1;
	} break;
	case "PRIME FACTOR CALCULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
		do {
			ret.inputs[0].clear();
			ret.n_outputs[0].clear();
			for ([[maybe_unused]] auto _ : kblib::range(10)) {
				word_t fac = 2;
				auto inp = ret.inputs[0].emplace_back(engine.next(10, 99));
				while (inp > fac) {
					if (inp % fac == 0) {
						ret.n_outputs[0].push_back(fac);
						inp = static_cast<word_t>(inp / fac);
					} else {
						++fac;
					}
				}
				ret.n_outputs[0].push_back(inp);
				ret.n_outputs[0].push_back(0);
			}
			// was this really the best way?
		} while (ret.n_outputs[0].size() != max_test_length - 1);
	} break;
	case "SIGNAL EXPONENTIATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);
		// extra 0 at the beginning because Lua arrays start at 1
		std::array<word_t, 11> max_exp{0, 10, 9, 6, 4, 4, 3, 3, 3, 3, 2};

		for ([[maybe_unused]] auto _ : kblib::range(max_test_length)) {
			auto a = ret.inputs[0].emplace_back(engine.next(1, 10));
			auto b = ret.inputs[1].emplace_back(engine.next(1, max_exp[a]));
			ret.n_outputs[0].push_back(static_cast<word_t>(std::pow(a, b)));
		}
	} break;
	case "T20 NODE EMULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2);
		ret.n_outputs.resize(1);

		auto& instructions = ret.inputs[0];
		auto& values = ret.inputs[1];
		auto& output = ret.n_outputs[0];
		instructions = {0, 1};
		values = {0, 0};

		word_t p = 0;
		word_t q = 0;

		for ([[maybe_unused]] auto _ : kblib::range(2, max_test_length)) {
			auto instr = engine.next(0, 4);
			instructions.push_back(instr);
			switch (instr) {
			case 0: {
				p = engine.next(10, 99);
				values.push_back(p);
			} break;
			case 1: {
				q = engine.next(10, 99);
				values.push_back(q);
			} break;
			case 2: {
				std::swap(p, q);
			} break;
			case 3: {
				p = p + q;
			} break;
			case 5:
			default: {
				output.push_back(p);
			} break;
			}
		}
	} break;
	case "T31 NODE EMULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);

		std::array<word_t, 8> memory{};
		std::array<bool, 8> written{};

		do {
			auto index = engine.next(0, 7);
			auto value = engine.next(10, 99);
			if (engine.next(0, 1)) {
				if (written[index]) {
					ret.inputs[0].push_back(1);
					ret.inputs[0].push_back(index);
					ret.n_outputs[0].push_back(memory[index]);
				}
			} else {
				ret.inputs[0].push_back(0);
				ret.inputs[0].push_back(index);
				ret.inputs[0].push_back(value);
				memory[index] = value;
				written[index] = true;
			}
		} while (ret.inputs[0].size() <= 36);
	} break;
	case "WAVE COLLAPSE SUPERVISOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(4, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		std::array<word_t, 4> sums{};

		for (const auto i : kblib::range(39u)) {
			for (const auto j : kblib::range(4u)) {
				auto n = engine.next(0, 1);
				if (i > 0 and ret.n_outputs[0][i - 1] == to_signed(j + 1)) {
					n = engine.next(-1, 0);
				}
				ret.inputs[j][i] = n;
				sums[j] = sums[j] + n;
			}
			auto max = std::ranges::max_element(sums);
			ret.n_outputs[0][i] = static_cast<word_t>(max - sums.begin() + 1);
		}
	} break;
	default:
		// no need to throw, the -L flag checks the range anyway
		assert(false);
	}

	auto log = log_debug();
	// I don't know why the game clamp negative values to -99, but it breaks
	// tests (segment 32050 seed 103061) so we're doing the sensible thing even
	// if it's wrong as far as a simulational versimilitude is concerned
	auto clamp = [](auto& vec) {
		std::ranges::transform(vec, vec.begin(), [](word_t v) {
			return std::clamp(v, word_min, word_max);
		});
	};
	for (auto& v : ret.inputs) {
		log << "Clamping in: ";
		write_list(log, v, nullptr, use_color and log_is_tty);
		clamp(v);
		log << " to ";
		write_list(log, v, nullptr, use_color and log_is_tty);
	}
	for (auto& v : ret.n_outputs) {
		log << "Clamping out: ";
		write_list(log, v, nullptr, use_color and log_is_tty);
		clamp(v);
		log << " to ";
		write_list(log, v, nullptr, use_color and log_is_tty);
		log << "\n";
	}

	return ret;
}
