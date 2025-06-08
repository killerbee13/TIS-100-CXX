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

#include "levels.hpp"
#include "image.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "tis_random.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

static word_vec make_random_array(xorshift128_engine& engine,
                                  std::uint32_t size, word_t min, word_t max) {
	word_vec array(size);
	for (std::uint32_t num = 0; num < size; ++num) {
		array[num] = engine.next_word(min, max);
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
		int sublistsize = engine.next_word(sublistmin, sublistmax);
		for (int i = 0; i < sublistsize; ++i) {
			list.push_back(engine.next_word(valuemin, valuemax));
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

static image_t checkerboard(std::ptrdiff_t w, std::ptrdiff_t h) {
	image_t ret(w, h);
	for (const auto y : range(h)) {
		for (const auto x : range(w)) {
			ret[x, y] = ((x ^ y) % 2) ? tis_pixel::C_black : tis_pixel::C_white;
		}
	}
	return ret;
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

inline word_vec empty_vec(word_t size = max_test_length) {
	assert(size >= 0);
	return word_vec(to_unsigned(size));
}

std::optional<single_test> builtin_level::random_test(uint32_t seed) {
	// log_info("random_test(", level_id, ", ", seed, ")");
	single_test ret{};
	switch (level_id) {
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
		ret.inputs[0][idx] = ret.inputs[1][idx] = engine.next_word(10, 100);
		ret.n_outputs.resize(1);
		for (const auto i : range(13u)) {
			auto [min, max] = std::minmax(ret.inputs[0][i], ret.inputs[1][i]);
			ret.n_outputs[0].push_back(min);
			ret.n_outputs[0].push_back(max);
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
				sum += w;
			}
		}
	} break;
	case "SIGNAL EDGE DETECTOR"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(empty_vec());
		ret.inputs[0][1] = engine.next_word(25, 75);

		for (std::size_t i = 2; i < max_test_length; i++) {
			switch (engine.next(0, 6)) {
			case 1:
				ret.inputs[0][i] = ret.inputs[0][i - 1] + engine.next_word(-11, -8);
				break;
			case 2:
				ret.inputs[0][i] = ret.inputs[0][i - 1] + engine.next_word(9, 12);
				break;
			default:
				ret.inputs[0][i] = ret.inputs[0][i - 1] + engine.next_word(-4, 5);
			}
		}

		ret.n_outputs.resize(1);
		word_t prev = 0;
		for (auto w : ret.inputs[0]) {
			ret.n_outputs[0].push_back(std::abs(w - std::exchange(prev, w)) >= 10);
		}
	} break;
	case "INTERRUPT HANDLER"_lvl: {
		ret.inputs.resize(4, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		std::array<bool, 4> vals{};

		xorshift128_engine engine(seed);
		for (std::size_t m = 1; m < max_test_length; ++m) {
			auto rand = engine.next(0, 6);
			if (rand < 4) {
				vals[rand] = not vals[rand];
				ret.n_outputs[0][m] = vals[rand] ? to_word(rand + 1) : 0;
			} else {
				ret.n_outputs[0][m] = 0;
			}
			for (std::size_t n = 0; n < 4; ++n) {
				ret.inputs[n][m] = vals[n];
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
			ret.inputs[0][num] = engine.next_word(1, 6);
			ret.inputs[0][num + 1] = 0;
			ret.inputs[0][num + 2] = 0;
			ret.inputs[0][num + 3] = engine.next_word(1, 6);
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
		ret.inputs[0][37] = engine.next_word(10, 100);
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
		ret.n_outputs = ret.inputs;

		for_each_subsequence_of(ret.n_outputs[0], 0, [&](auto begin, auto end) {
			std::reverse(begin, end);
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
		ret.i_outputs.emplace_back(image_width, image_height, tis_pixel::C_white);
	} break;
	case "IMAGE TEST PATTERN 2"_lvl: {
		ret.i_outputs.push_back(checkerboard(image_width, image_height));
	} break;
	case "EXPOSURE MASK VIEWER"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.resize(1);
		auto& image = ret.i_outputs.emplace_back(image_width, image_height);
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
					log_trace("skipped while placing rectangle ", i);
					return std::nullopt;
				}
				w = engine.next_word(3, 6);
				h = engine.next_word(3, 6);
				x_c = engine.next_word(1, image_width - 1 - w);
				y_c = engine.next_word(1, image_height - 1 - h);
				// Check if the rectangle would overlap or touch any already placed
				// rectangle
				for (int k = -1; k < h + 1; ++k) {
					for (int j = -1; j < w + 1; ++j) {
						if (image[x_c + j, y_c + k] != tis_pixel::C_black) {
							++iterations;
							goto retry;
						}
					}
				}
			}

			ret.inputs[0].push_back(x_c);
			ret.inputs[0].push_back(y_c);
			ret.inputs[0].push_back(w);
			ret.inputs[0].push_back(h);
			for (int k = 0; k < h; ++k) {
				for (int j = 0; j < w; ++j) {
					image[x_c + j, y_c + k] = tis_pixel::C_white;
				}
			}
			log_debug_r([&] { return "image:\n" + image.write_text(); });
		}
	} break;
	case "HISTOGRAM VIEWER"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(empty_vec(image_width));
		ret.i_outputs.emplace_back(image_width, image_height);
		ret.inputs[0][0] = engine.next_word(3, 14);
		for (std::size_t x = 1; x < image_width; ++x) {
			if (engine.next(0, 4) != 0) {
				ret.inputs[0][x] = std::clamp<word_t>(ret.inputs[0][x - 1]
				                                          + engine.next_word(-2, 3),
				                                      1, image_height - 1);
			} else {
				ret.inputs[0][x] = engine.next_word(3, 14);
			}
		}
		for (int x = 0; x < image_width; ++x) {
			for (int y = image_height - ret.inputs[0][x]; y < image_height; ++y) {
				ret.i_outputs[0][x, y] = tis_pixel::C_white;
			}
		}
	} break;
	case "IMAGE CONSOLE SANDBOX"_lvl: {
		ret.inputs.resize(1);
		ret.i_outputs.emplace_back(36, 22);
	} break;
	case "SIGNAL WINDOW FILTER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
		ret.n_outputs = {empty_vec(), empty_vec()};
		word_t t3 = 0, t5 = 0;
		for (std::size_t idx = 0; idx < max_test_length; ++idx) {
			t3 += ret.inputs[0][idx];
			t5 += ret.inputs[0][idx];
			if (idx >= 3) {
				t3 -= ret.inputs[0][idx - 3];
			}
			if (idx >= 5) {
				t5 -= ret.inputs[0][idx - 5];
			}
			ret.n_outputs[0][idx] = t3;
			ret.n_outputs[1][idx] = t5;
		}
	} break;
	case "SIGNAL DIVIDER"_lvl: {
		ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
		ret.inputs.push_back(make_random_array(seed + 1, max_test_length, 1, 10));
		ret.n_outputs.resize(2, empty_vec());
		for (std::size_t i = 0; i < max_test_length; ++i) {
			ret.n_outputs[0][i] = to_word(ret.inputs[0][i] / ret.inputs[1][i]);
			ret.n_outputs[1][i] = to_word(ret.inputs[0][i] % ret.inputs[1][i]);
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
		ret.n_outputs = ret.inputs;

		for_each_subsequence_of(ret.n_outputs[0], 0, [&](auto begin, auto end) {
			std::ranges::sort(begin, end);
		});
	} break;
	case "STORED IMAGE DECODER"_lvl: {
		// the tests for this level are AWFUL, the first time you complete the
		// level a cutscene test plays, which uses the following input:
		// {270, 1, 2,  2, 28, 1, 3,  2, 27, 1, 3,   2, 27,
		//    1, 4, 2, 26,  1, 5, 2, 27,  1, 3, 2, 115,  1}
		// after the first win, the game SHOULD use normal random tests, but a bug
		// makes it so the seed used is always `21 advance 1`,
		// which results in the following input:
		// {41, 2, 33, 2, 39, 1, 35, 2, 23, 1, 30, 1, 34, 2, 20, 3, 36, 0,
		//  38, 0, 25, 1, 24, 2, 31, 0, 22, 1, 29, 1, 30, 0, 32, 1, 20, 0}
		// this is the test everyone sees all the time
		// the sim runs the intended tests, which are implemented below:
		xorshift128_engine engine(seed);
		ret.inputs.resize(1);
		// this can theoretically generate up to W*H/20*2 = 54 input values,
		// sizes up to 46 have been observed (seed 2955698), we just run with an
		// oversized test in those case
		const size_t image_size = image_width * image_height;
		ret.inputs[0].reserve(image_size / 20 * 2);
		std::vector<tis_pixel> image(image_size + 45);

		auto im_it = image.begin();
		while (to_unsigned(im_it - image.begin()) < image_size) {
			word_t count = engine.next_word(20, 45);
			word_t pix = engine.next_word(0, 4);
			ret.inputs[0].push_back(count);
			ret.inputs[0].push_back(pix);
			im_it = std::fill_n(im_it, to_unsigned(count), tis_pixel(pix));
		}
		if (ret.inputs[0].size() > max_test_length) {
			log_debug("Oversized test of size: ", ret.inputs[0].size(),
			          " for seed: ", seed);
		}
		image.resize(image_size);
		ret.i_outputs.emplace_back(image_width, image_height, std::move(image));
	} break;
	case "UNKNOWN"_lvl: {
		xorshift128_engine engine(seed);
		ret.inputs.push_back(empty_vec());
		ret.n_outputs.resize(2);
		while (ret.n_outputs[0].size() < max_test_length) {
			word_t item = engine.next_word(0, 4);
			uint size = engine.next(2, 5);
			ret.n_outputs[0].insert(ret.n_outputs[0].end(), size, item);
		}
		ret.n_outputs[0].resize(max_test_length);
		for (std::size_t j = 0; j < max_test_length; ++j) {
			ret.inputs[0][j]
			    = to_word(ret.n_outputs[0][j] * 25 + 12 + engine.next_word(-6, 7));
		}
		ret.n_outputs[0].back() = -1;
		ret.inputs[0].back() = -1;
		word_t prev = -1;
		word_t count = 0;
		for (auto curr : ret.n_outputs[0]) {
			if (prev != curr) {
				if (prev >= 0) {
					ret.n_outputs[1].push_back(count);
					ret.n_outputs[1].push_back(prev);
				}
				prev = curr;
				count = 1;
			} else {
				++count;
			}
		}
	} break;
	case "SEQUENCE MERGER"_lvl: {
		lua_random engine(to_signed(seed));
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
					maxout = engine.next_word(0, to_word(maxmax));
				} while (not canzero and maxout == 0);
			}

			std::size_t count1;
			if (prevempty and maxout >= 2) {
				count1 = engine.next_word(1, to_word(maxout - 1));
			} else {
				count1 = engine.next_word(0, to_word(maxout));
			}
			if (maxout == 0) {
				canzero = false;
			}

			prevempty = (count1 == 0 or count1 == maxout);
			if (maxout > 0) {
				word_vec outseq(maxout);
				word_vec in1seq(count1);
				word_vec in2seq(maxout - count1);
				for (std::size_t i = 0; i < maxout; i++) {
					word_t val;
					do {
						val = engine.next_word(10, 99);
					} while (std::ranges::contains(outseq, val));
					outseq[i] = val;
					if (i < count1) {
						in1seq[i] = val;
					} else {
						in2seq[i - count1] = val;
					}
				}
				std::ranges::sort(outseq);
				std::ranges::sort(in1seq);
				std::ranges::sort(in2seq);
				std::ranges::copy(outseq, std::back_inserter(out));
				std::ranges::copy(in1seq, std::back_inserter(ret.inputs[0]));
				std::ranges::copy(in2seq, std::back_inserter(ret.inputs[1]));
			}
			out.push_back(0);
			ret.inputs[0].push_back(0);
			ret.inputs[1].push_back(0);
		} while (out.size() < max_test_length);
	} break;
	case "INTEGER SERIES CALCULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		for (std::size_t i = 0; i < max_test_length; i++) {
			word_t n = engine.next_word(1, 44);
			ret.inputs[0][i] = n;
			ret.n_outputs[0][i] = to_word(n * (n + 1) / 2);
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
			mininput.push_back(engine.next_word(3, 9) * 5);
		}
		// make maximums big
		for (std::size_t i = 0; i < 6; i++) {
			maxinput.push_back(engine.next_word(10, 17) * 5);
		}
		// For now, just make five sequences that are five long (with a
		// sixth value being a 0 terminator)
		// Manually calculate correct output values
		for (std::size_t i = 0; i < 6; i++) {
			for (std::size_t j = 0; j < 5; j++) {
				word_t val = engine.next_word(10, 99);
				input.push_back(val);
				output.push_back(std::clamp(val, mininput[i], maxinput[i]));
			}
			input.push_back(0);
			output.push_back(0);
		}
	} break;
	case "SIGNAL ERROR CORRECTOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2, empty_vec());
		ret.n_outputs.resize(2, empty_vec());
		word_vec& in_a = ret.inputs[0];
		word_vec& in_b = ret.inputs[1];
		word_vec& out_a = ret.n_outputs[0];
		word_vec& out_b = ret.n_outputs[1];
		for (int i = 0; i < max_test_length; i++) {
			word_t r = engine.next_word(1, 4);
			word_t a = engine.next_word(10, 99);
			word_t b = engine.next_word(10, 99);
			switch (r) {
			case 1: {
				in_a[i] = -1;
				in_b[i] = b;
				out_a[i] = b;
				out_b[i] = b;
				break;
			}
			case 2: {
				in_a[i] = a;
				in_b[i] = -1;
				out_a[i] = a;
				out_b[i] = a;
				break;
			}
			default: {
				in_a[i] = a;
				in_b[i] = b;
				out_a[i] = a;
				out_b[i] = b;
				break;
			}
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
			std::size_t j = engine.next_word(0, to_word(i));
			std::swap(seq_lengths[i], seq_lengths[j]);
		}

		for (word_t len : seq_lengths) {
			// Generate input sequences:
			for (word_t j = 0; j < len; j++) {
				in_seq.push_back(engine.next_word(10, 99));
			}
			in_seq.push_back(0);
			// Generate subsequence indexes:
			word_t sublen = engine.next_word(2, len);
			word_t first = engine.next_word(0, len - sublen);
			word_t last = to_word(first + sublen - 1);
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
		ret.inputs.resize(1, empty_vec());
		ret.n_outputs.resize(3, empty_vec());
		for (int i = 0; i < max_test_length; i++) {
			word_t val = engine.next_word(1, 120);
			ret.n_outputs[2][i] = val;
			ret.n_outputs[1][i] = val * 2;
			ret.n_outputs[0][i] = val * 4;
			ret.inputs[0][i] = val * 8;
		}
	} break;
	case "SIGNAL AVERAGER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		for (int i = 0; i < max_test_length; i++) {
			word_t valA = engine.next_word(100, 999);
			word_t valB = engine.next_word(100, 999);
			ret.inputs[0][i] = valA;
			ret.inputs[1][i] = valB;
			ret.n_outputs[0][i] = to_word((valA + valB) / 2);
		}
	} break;
	case "SUBMAXIMUM SELECTOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(4, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		for (int i = 0; i < max_test_length; i++) {
			std::array<word_t, 4> group;
			for (std::size_t j = 0; j < 4; j++) {
				word_t v = engine.next_word(0, 99);
				group[j] = v;
				ret.inputs[j][i] = v;
			}

			std::ranges::nth_element(group, group.begin() + 2);
			ret.n_outputs[0][i] = group[2];
		}
	} break;
	case "DECIMAL DECOMPOSER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1, empty_vec());
		ret.n_outputs.resize(3, empty_vec());
		for (int i = 0; i < max_test_length; i++) {
			word_t digits = engine.next_word(0, 2);
			word_t val;
			if (digits == 0) {
				val = engine.next_word(0, 9);
			} else if (digits == 1) {
				val = engine.next_word(10, 99);
			} else {
				val = engine.next_word(100, 999);
			}
			ret.inputs[0][i] = val;
			ret.n_outputs[0][i] = val / 100;
			ret.n_outputs[1][i] = (val % 100) / 10;
			ret.n_outputs[2][i] = val % 10;
		}
	} break;
	case "SEQUENCE MODE CALCULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1, empty_vec());
		ret.n_outputs.resize(1);

		// generate input stream
		int last_zero = -1;
		for (int i = 0; i < max_test_length - 1; i++) {
			ret.inputs[0][i] = engine.next_word(1, 5);
			// tuned to give nice balance of sequence lengths
			if (i - last_zero > 3 and engine.next_double() < 0.5
			    and i < max_test_length - 2) {
				ret.inputs[0][i] = 0;
				last_zero = i;
			}
		}
		ret.inputs[0].back() = 0;

		// generate output stream
		std::array frequency = {0u, 0u, 0u, 0u, 0u};
		for (word_t input : ret.inputs[0]) {
			if (input == 0) {
				// determine mode, and whether it is unique
				uint max_frequency = 0;
				uint most_frequent = 0;
				for (uint k = 0; k < 5; k++) {
					if (frequency[k] > max_frequency) {
						most_frequent = k + 1;
						max_frequency = frequency[k];
					} else if (frequency[k] == max_frequency) {
						most_frequent = 0; // "error" if not unique
					}
				}
				ret.n_outputs[0].push_back(to_word(most_frequent));
				frequency.fill(0u);
			} else {
				// update frequency map
				frequency[input - 1]++;
			}
		}
	} break;
	case "SEQUENCE NORMALIZER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1, empty_vec(max_test_length - 1));
		ret.n_outputs.resize(1, empty_vec(max_test_length - 1));

		int curr_start = 0;
		for (int i = 0; i < max_test_length - 1; i++) {
			word_t val = engine.next_word(1, 99);
			ret.inputs[0][i] = val;
			ret.n_outputs[0][i] = val;

			auto cur_seq = std::span(ret.n_outputs[0])
			                   .subspan(curr_start, i - curr_start + 1);
			// Possibly end sequence ensuring that the it is at least of length 3
			// and no longer than 8.
			// Also ensure final sequence is ended properly - may cause shorter
			// sequence length.
			if ((engine.next_word(1, 3) == 3 and cur_seq.size() > 2)
			    or (cur_seq.size() > 7) or (i == max_test_length - 3)) {
				// Generate 'output'
				word_t min_in_seq = std::ranges::min(cur_seq);
				for (word_t& seqval : cur_seq) {
					seqval -= min_in_seq;
				}

				// Add the sequence terminating -1
				i++;
				ret.inputs[0][i] = -1;
				ret.n_outputs[0][i] = -1;
				// Update position of next sequence start
				curr_start = i + 1;
			}
		}
		// Clean up any unfinished sequence
		ret.n_outputs[0].resize(curr_start);
	} break;
	case "IMAGE TEST PATTERN 3"_lvl: {
		ret.i_outputs.emplace_back<image_t>({
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
		ret.i_outputs.emplace_back<image_t>({
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
		ret.i_outputs.emplace_back(image_width, image_height);

		// Helper method
		auto makeCoords = [&engine](std::size_t size, word_t max) {
			// fill
			std::vector<word_t> coors(max + 1);
			for (word_t i = 0; i < max + 1; i++) {
				coors[i] = i;
			}
			// shuffle, aside from the 1st that has to remain 0
			for (std::size_t i = max; i > 1; i--) {
				// i is now the last pertinent index
				std::size_t k = engine.next_word(1, to_word(i));
				// Quick swap
				std::swap(coors[i], coors[k]);
			}
			// place valid coords at the beginning of the vector
			std::size_t good = 1;
			for (std::size_t i = good; i < coors.size(); i++) {
				int d = std::abs(coors[good - 1] - coors[i]);
				if (d >= 3 and d <= 14) {
					std::rotate(coors.begin() + good, coors.begin() + i,
					            coors.begin() + i + 1);
					good++;
					if (good == size) {
						break;
					} else {
						i = good - 1;
					}
				}
			}
			coors.resize(size);
			return coors;
		};

		// Construct set of 11 points where
		//		no two points share an X or Y coordinate, and
		//		no adjacent points have X or Y coordinates less than 4 or more than
		// 15 apart.
		std::size_t size = 11;
		auto coorsX = makeCoords(size, image_width - 1);
		auto coorsY = makeCoords(size, image_height - 1);

		// Draw lines, alternating from horizontal to vertical, between each
		// adjacent pair of points.
		for (std::size_t i = 1; i < size; i++) {
			auto xOne = coorsX[i - 1];
			auto xTwo = coorsX[i];
			auto yOne = coorsY[i - 1];
			auto yTwo = coorsY[i];

			word_t dx;
			if (xTwo < xOne) {
				ret.inputs[0].push_back(180);
				dx = -1;
			} else {
				ret.inputs[0].push_back(0);
				dx = 1;
			}
			for (word_t x = xOne; x != xTwo + dx; x += dx) {
				ret.i_outputs[0][x, yOne] = tis_pixel::C_white;
			}
			ret.inputs[0].push_back(to_word(std::abs(xOne - xTwo) + 1));

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
				ret.i_outputs[0][xTwo, y] = tis_pixel::C_white;
			}
			ret.inputs[0].push_back(to_word(std::abs(yOne - yTwo) + 1));
		}
	} break;
	case "CHARACTER TERMINAL"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.i_outputs.emplace_back(image_width, image_height);

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
						ret.i_outputs[0][x + a, y + b] = tis_pixel::C_white;
					}
				}
			}
		};

		auto& input = ret.inputs[0];
		for (std::size_t i = 0; i < max_test_length; i++) {
			input.push_back(engine.next_word(1, 4));
		}
		input.push_back(0);

		// these values are choosen to allow a bit of undefined behavior; I do not
		// pick whether a 0 at after the 10th character should be an empty line.
		// The prerolled sample will not have that, so I avoid testing the user on
		// that case because it seemed unfair.  I think both ways could be valid.
		input[engine.next_word(12, 16)] = 0;
		input[engine.next_word(28, 31)] = 0;

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
	} break;
	case "BACK-REFERENCE REIFIER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		auto& input_refs = ret.inputs[0];
		auto& input_values = ret.inputs[1];
		for (int i = 0; i < max_test_length; i++) {
			word_t ref = 0;
			if (engine.next_word(0, 1) == 0) {
				ref = engine.next_word(-4, -1);
				if (i + ref < 0) {
					ref = 0;
				}
			}
			input_values[i] = engine.next_word(10, 99);
			input_refs[i] = ref;
			ret.n_outputs[0][i] = input_values[i + ref];
		}
	} break;
	case "DYNAMIC PATTERN DETECTOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs = {word_vec(4), word_vec(max_test_length)};
		ret.n_outputs.push_back(word_vec(max_test_length));
		auto& pattern = ret.inputs[0];
		auto& input = ret.inputs[1];
		auto& output = ret.n_outputs[0];

		for (int i = 0; i < 12; i++) {
			// RNG manip (There has to be a 42 in the pattern.)
			// NOTE: there is not, because the custom puzzle seed is different
			engine.next_double();
		}

		for (int i = 0; i < 3; i++) {
			pattern[i] = engine.next_word(1, 42); // PATTERN gen
		}
		pattern.back() = 0;
		for (int i = 0; i < max_test_length; i++) {
			input[i] = engine.next_word(1, 42); // SEQUENCE gen
		}

		for (int k = 0; k < 2; k++) {
			// pattern 123 potential extra matches
			word_t j = engine.next_word(1, 37);
			for (int i = 0; i < 3; i++) {
				input[i + j - 1] = pattern[i];
			}
		}
		for (int k = 0; k < 3; k++) {
			// pattern 23 these may be overwritten
			word_t j = engine.next_word(1, 37);
			for (int i = 1; i < 3; i++) {
				input[i + j - 1] = pattern[i];
			}
		}

		// The following are guaranteed to be complete ( do I need a 123123? )
		word_t j = engine.next_word(1, 7); // pattern 123
		for (int i = 0; i < 3; i++) {
			input[i + j - 1] = pattern[i];
		}

		// pattern 1223 Don't know if guaranteed X23 is also needed.
		j = engine.next_word(10, 13);
		for (int i = 0; i < 2; i++) {
			input[i + j - 1] = pattern[i];
		}
		for (int i = 1; i < 3; i++) {
			input[i + j] = pattern[i];
		}

		j = engine.next_word(17, 23); // pattern 1123
		input[j - 1] = pattern[0];
		for (int i = 0; i < 3; i++) {
			input[j + i] = pattern[i];
		}

		j = engine.next_word(27, 35); // pattern 12123
		input[j - 1] = pattern[0];
		input[j] = pattern[1];
		for (int i = 0; i < 3; i++) {
			input[j + i + 1] = pattern[i];
		}

		output[0] = 0;
		output[1] = 0;
		for (int i = 2; i < max_test_length; i++) {
			if (input[i - 2] == pattern[0] and input[i - 1] == pattern[1]
			    and input[i] == pattern[2]) {
				output[i] = 1;
			} else {
				output[i] = 0;
			}
		}
	} break;
	case "SEQUENCE GAP INTERPOLATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
		auto& in = ret.inputs[0];
		in.reserve(max_test_length);

		std::array lengths{5, 4, 4, 4, 5, 4, 5, 4, 4};
		for (auto length : lengths) {
			// Figure out the min, max, and missing values:
			word_t min = engine.next_word(10, 90);
			word_t max = to_word(min + length - 1);
			auto missing_value = engine.next_word(min + 1, max - 1);
			// Insert the values into the stream
			auto start = std::ssize(in);
			for (auto i : range<word_t>(min, max + 1)) {
				if (i != missing_value) {
					in.push_back(i);
				}
			}
			// Shuffle list in place
			for (auto i : range(std::ssize(in) - 1, start, -1)) {
				auto j = engine.next_word(to_word(start), to_word(i));
				std::swap(in[i], in[j]);
			}
			in.push_back(0);
			ret.n_outputs[0].push_back(missing_value);
		}
	} break;
	case "DECIMAL TO OCTAL CONVERTER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		auto to_octal = [](word_t i) { return (i / 8) * 10 + (i % 8); };

		for (auto i : range(max_test_length)) {
			auto v = ret.inputs[0][i] = engine.next_word(1, 63);
			ret.n_outputs[0][i] = to_word(to_octal(v));
		}
	} break;
	case "PROLONGED SEQUENCE SORTER"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1, empty_vec());

		// I want to force at least 1 number to not appear
		// otherwise there's a few shortcuts you can take
		std::array<bool, 10> seen{};
		int zeros = 10;
		for (std::size_t i = 0; i < max_test_length - 1; ++i) {
			do {
				ret.inputs[0][i] = engine.next_word(0, 9);
			} while (zeros == 1 and not seen[ret.inputs[0][i]]);

			if (not seen[ret.inputs[0][i]]) {
				seen[ret.inputs[0][i]] = true;
				--zeros;
			}
		}
		ret.inputs[0].back() = -1;

		ret.n_outputs = ret.inputs;
		std::ranges::sort(ret.n_outputs[0].begin(), ret.n_outputs[0].end() - 1);
	} break;
	case "PRIME FACTOR CALCULATOR"_lvl: {
		// the algorithm used in the actual game spec is extremely slow, as it
		// takes an average of 15 tries of the do-while to produce a test, which
		// are 150 random calls
		// so optimize out everything else, by precomputing the prime
		// factorization and minimizing vector checks and copies
		// random test generation is still over half of the runtime of a 100
		// cycles solve, but the vast majority of that is randomness calls
		static const std::array<word_vec, 100> cache = [] {
			std::array<word_vec, 100> res;
			for (word_t inp : range(word_t{10}, word_t{100})) {
				word_vec& factors = res[inp];
				word_t fac = 2;
				while (inp >= fac * fac) {
					if (inp % fac == 0) {
						factors.push_back(fac);
						inp = to_word(inp / fac);
					} else {
						++fac;
					}
				}
				factors.push_back(inp);
			}
			return res;
		}();

		lua_random engine(to_signed(seed));
		ret.inputs.resize(1, empty_vec(10));
		ret.n_outputs.resize(1, empty_vec(max_test_length - 1));
		int sum;
		do {
			sum = 0;
			for (auto i : range(10)) {
				auto inp = ret.inputs[0][i] = engine.next_word(10, 99);
				sum += static_cast<int>(cache[inp].size()) + 1;
			}
		} while (sum != max_test_length - 1);

		auto it = ret.n_outputs[0].begin();
		for (word_t inp : ret.inputs[0]) {
			it = std::ranges::copy(cache[inp], it).out;
			it++; // 0
		}
	} break;
	case "SIGNAL EXPONENTIATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(2, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		// extra 0 at the beginning because Lua arrays start at 1
		std::array<word_t, 11> max_exp{0, 10, 9, 6, 4, 4, 3, 3, 3, 3, 2};

		for (auto i : range(max_test_length)) {
			auto a = ret.inputs[0][i] = engine.next_word(1, 10);
			auto b = ret.inputs[1][i] = engine.next_word(1, max_exp[a]);
			ret.n_outputs[0][i] = to_word(std::pow(a, b));
		}
	} break;
	case "T20 NODE EMULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs = {empty_vec(), word_vec()};
		ret.n_outputs.resize(1);

		auto& instructions = ret.inputs[0];
		instructions[0] = 0;
		instructions[1] = 1;
		auto& values = ret.inputs[1];
		values = {0, 0};

		word_t p = 0;
		word_t q = 0;

		for (auto i : range(2, max_test_length)) {
			auto instr = engine.next_word(0, 4);
			instructions[i] = instr;
			switch (instr) {
			case 0: {
				p = engine.next_word(10, 99);
				values.push_back(p);
			} break;
			case 1: {
				q = engine.next_word(10, 99);
				values.push_back(q);
			} break;
			case 2: {
				std::swap(p, q);
			} break;
			case 3: {
				p += q;
			} break;
			default: {
				ret.n_outputs[0].push_back(p);
			} break;
			}
		}
	} break;
	case "T31 NODE EMULATOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(1);
		ret.n_outputs.resize(1);
		ret.inputs[0].reserve(max_test_length);

		std::array<word_t, 8> memory{};

		do {
			auto index = engine.next_word(0, 7);
			auto value = engine.next_word(10, 99);
			if (engine.next_word(0, 1)) {
				if (memory[index]) { // 0 can be used as sentinel
					ret.inputs[0].push_back(1);
					ret.inputs[0].push_back(index);
					ret.n_outputs[0].push_back(memory[index]);
				}
			} else {
				ret.inputs[0].push_back(0);
				ret.inputs[0].push_back(index);
				ret.inputs[0].push_back(value);
				memory[index] = value;
			}
		} while (ret.inputs[0].size() <= 36);
	} break;
	case "WAVE COLLAPSE SUPERVISOR"_lvl: {
		lua_random engine(to_signed(seed));
		ret.inputs.resize(4, empty_vec());
		ret.n_outputs.resize(1, empty_vec());
		std::array<word_t, 4> sums{};

		for (const auto i : range(max_test_length)) {
			for (const auto j : range(4)) {
				auto n = engine.next_word(0, 1);
				if (i > 0 and ret.n_outputs[0][i - 1] == j + 1) {
					n = engine.next_word(-1, 0);
				}
				ret.inputs[j][i] = n;
				sums[j] += n;
			}
			auto max = std::ranges::max_element(sums);
			ret.n_outputs[0][i] = to_word(max - sums.begin() + 1);
		}
	} break;
	default:
		// no need to throw, the -L flag checks the range anyway
		std::unreachable();
	}

	clamp_test_values(ret);
	return ret;
}
