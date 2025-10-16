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

#include "T21.hpp"
#include "T30.hpp"
#include "field.hpp"
#include "game.hpp"
#include "image.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "tests.hpp"
#include "tis_random.hpp"

#include <kblib/hash.h>
#include <kblib/io.h>

#include <algorithm>
#include <iterator>
#include <optional>
#include <span>
#include <vector>
#include <memory>

field builtin_level::new_field(uint T30_size) const {
	return field(layout, T30_size);
}
std::unique_ptr<level> builtin_level::clone() const {
	return std::make_unique<builtin_level>(*this);
}

bool builtin_level::has_achievement(const field& solve, const score& sc) const {
	auto debug = log_debug();
	debug << "check_achievement " << name << ": ";
	switch (kblib::FNV32a(segment)) {
	case "00150"_fnv32: { // SELF-TEST DIAGNOSTIC
		debug << "BUSY_LOOP: " << sc.cycles << ((sc.cycles > 100000) ? ">" : "<=")
		      << 100000;
		return sc.cycles > 100000;
	}
	case "21340"_fnv32: { // SIGNAL COMPARATOR
		debug << "UNCONDITIONAL:\n";
		for (auto& n : solve.regulars()) {
			if (n->type == node::T21) {
				auto p = static_cast<const T21*>(n.get());
				debug << "T20 (" << p->x << ',' << p->y << "): ";
				if (p->code.empty()) {
					debug << "empty";
				} else if (p->has_instr(instr::jez, instr::jnz, instr::jgz,
				                        instr::jlz)) {
					debug << " conditional found";
					return false;
				}
				debug << '\n';
			}
		}
		debug << " no conditionals found";
		return true;
	}
	case "42656"_fnv32: { // SEQUENCE REVERSER
		debug << "NO_MEMORY: ";
		for (auto& n : solve.regulars()) {
			if (n->type == node::T30) {
				auto p = static_cast<const T30*>(n.get());
				debug << "T30 (" << p->x << ',' << p->y << "): " << p->used << '\n';
				if (p->used) {
					return false;
				}
			}
		}
		debug << "no stacks used";
		return true;
	}
	default: {
		debug << "no achievement";
		return false;
	}
	}
}

std::unique_ptr<builtin_level> builtin_level::from_name(std::string_view s) {
	for (auto& l : builtin_levels) {
		if (s == l.segment or s == l.name) {
			return std::make_unique<builtin_level>(l);
		}
	}
	throw std::invalid_argument{concat("invalid level ID ", kblib::quoted(s))};
}

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

static word_vec make_composite_array(xorshift128_engine& engine, uint size,
                                     uint sublistmin, uint sublistmax,
                                     word_t valuemin, word_t valuemax) {
	word_vec list;
	list.reserve(size + sublistmax);
	while (list.size() < size) {
		uint sublistsize = engine.next(sublistmin, sublistmax);
		for (uint i = 0; i < sublistsize; ++i) {
			list.push_back(engine.next_word(valuemin, valuemax));
		}
		list.push_back(0);
	}
	if (list.size() > size) {
		list.erase(list.begin() + size, list.end());
		list.back() = 0;
	}
	return list;
}
static word_vec make_composite_array(uint seed, uint size, uint sublistmin,
                                     uint sublistmax, word_t valuemin,
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
static void for_each_subsequence_of(word_vec& in, word_t delim, F f) {
	auto start = in.begin();
	auto cur = start;
	for (; cur != in.end(); ++cur) {
		if (*cur == delim) {
			f(start, cur);
			start = cur + 1;
		}
	}
}

static word_vec zero_vec(word_t size = max_test_length) {
	assert(size >= 0);
	return word_vec(to_unsigned(size));
}

using maybe_test = std::optional<single_test>;

static maybe_test random_test_self_test_diagnostic(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
	ret.inputs.push_back(make_random_array(seed + 1, max_test_length, 10, 100));
	ret.n_outputs = ret.inputs;
	return ret;
}
static maybe_test random_test_signal_amplifier(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
	ret.n_outputs.resize(1, zero_vec());
	std::ranges::transform(ret.inputs[0], ret.n_outputs[0].begin(),
	                       [](word_t x) { return 2 * x; });
	return ret;
}
static maybe_test random_test_differential_converter(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
	ret.inputs.push_back(make_random_array(seed + 1, max_test_length, 10, 100));
	ret.n_outputs.resize(2, zero_vec());
	std::ranges::transform(ret.inputs[0], ret.inputs[1],
	                       ret.n_outputs[0].begin(),
	                       [](word_t x, word_t y) { return x - y; });
	std::ranges::transform(ret.inputs[0], ret.inputs[1],
	                       ret.n_outputs[1].begin(),
	                       [](word_t x, word_t y) { return y - x; });
	return ret;
}
static maybe_test random_test_signal_comparator(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(make_random_array(seed, max_test_length, -2, 3));
	ret.n_outputs.resize(3, zero_vec());
	for (auto [x, i] : kblib::enumerate(ret.inputs[0])) {
		ret.n_outputs[0][i] = (x > 0);
		ret.n_outputs[1][i] = (x == 0);
		ret.n_outputs[2][i] = (x < 0);
	}
	return ret;
}
static maybe_test random_test_signal_multiplexer(uint32_t seed) {
	single_test ret{};
	ret.inputs.resize(3);
	ret.n_outputs.resize(1, zero_vec());
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
	clamp_test_values(ret);
	return ret;
}
static maybe_test random_test_sequence_generator(uint32_t seed) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_sequence_counter(uint32_t seed) {
	single_test ret{};
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
	clamp_test_values(ret);
	return ret;
}
static maybe_test random_test_signal_edge_detector(uint32_t seed) {
	single_test ret{};
	xorshift128_engine engine(seed);
	ret.inputs.push_back(zero_vec());
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

	ret.n_outputs.resize(1, zero_vec());
	word_t prev = 0;
	for (auto [w, i] : kblib::enumerate(ret.inputs[0])) {
		ret.n_outputs[0][i] = std::abs(w - std::exchange(prev, w)) >= 10;
	}
	clamp_test_values(ret);
	return ret;
}
static maybe_test random_test_interrupt_handler(uint32_t seed) {
	single_test ret{};
	ret.inputs.resize(4, zero_vec());
	ret.n_outputs.resize(1, zero_vec());
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
	return ret;
}
static maybe_test random_test_simple_sandbox(uint32_t) {
	single_test ret{};
	ret.inputs.resize(1);
	ret.n_outputs.resize(1);
	return ret;
}
static maybe_test random_test_signal_pattern_detector(uint32_t seed) {
	single_test ret{};
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
	ret.n_outputs.push_back(zero_vec());
	for (std::size_t j = 0; j < max_test_length; ++j) {
		ret.n_outputs[0][j]
		    = (j > 1 and ret.inputs[0][j - 2] == 0 and ret.inputs[0][j - 1] == 0
		       and ret.inputs[0][j] == 0);
	}
	return ret;
}
static maybe_test random_test_sequence_peak_detector(uint32_t seed) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_sequence_reverser(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(
	    make_composite_array(seed, max_test_length, 0, 6, 10, 100));
	ret.n_outputs = ret.inputs;

	for_each_subsequence_of(ret.n_outputs[0], 0, [&](auto begin, auto end) {
		std::reverse(begin, end);
	});
	return ret;
}
static maybe_test random_test_signal_multiplier(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(make_random_array(seed, max_test_length, 0, 10));
	ret.inputs.push_back(make_random_array(seed + 1, max_test_length, 0, 10));
	ret.n_outputs.push_back(zero_vec());
	std::ranges::transform(ret.inputs[0], ret.inputs[1],
	                       ret.n_outputs[0].begin(), std::multiplies{});
	return ret;
}
static maybe_test random_test_stack_memory_sandbox(uint32_t) {
	single_test ret{};
	ret.inputs.resize(1);
	ret.n_outputs.resize(1);
	return ret;
}
static maybe_test random_test_image_test_pattern_1(uint32_t) {
	single_test ret{};
	ret.i_outputs.emplace_back(image_width, image_height, tis_pixel::C_white);
	return ret;
}
static maybe_test random_test_image_test_pattern_2(uint32_t) {
	single_test ret{};
	ret.i_outputs.push_back(checkerboard(image_width, image_height));
	return ret;
}
static maybe_test random_test_exposure_mask_viewer(uint32_t seed) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_histogram_viewer(uint32_t seed) {
	single_test ret{};
	xorshift128_engine engine(seed);
	ret.inputs.push_back(zero_vec(image_width));
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
	return ret;
}
static maybe_test random_test_image_console_sandbox(uint32_t) {
	single_test ret{};
	ret.inputs.resize(1);
	ret.i_outputs.emplace_back(36, 22);
	return ret;
}
static maybe_test random_test_signal_window_filter(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
	ret.n_outputs = {zero_vec(), zero_vec()};
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
	return ret;
}
static maybe_test random_test_signal_divider(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(make_random_array(seed, max_test_length, 10, 100));
	ret.inputs.push_back(make_random_array(seed + 1, max_test_length, 1, 10));
	ret.n_outputs.resize(2, zero_vec());
	for (std::size_t i = 0; i < max_test_length; ++i) {
		ret.n_outputs[0][i] = to_word(ret.inputs[0][i] / ret.inputs[1][i]);
		ret.n_outputs[1][i] = to_word(ret.inputs[0][i] % ret.inputs[1][i]);
	}
	return ret;
}
static maybe_test random_test_sequence_indexer(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(make_random_array(seed, 10, 100, 1000));
	ret.inputs[0].push_back(0);
	ret.inputs.push_back(make_random_array(seed, max_test_length, 0, 10));
	ret.n_outputs.resize(1, zero_vec());
	for (std::size_t i = 0; i < max_test_length; ++i) {
		ret.n_outputs[0][i] = ret.inputs[0][to_unsigned(ret.inputs[1][i])];
	}
	return ret;
}
static maybe_test random_test_sequence_sorter(uint32_t seed) {
	single_test ret{};
	ret.inputs.push_back(
	    make_composite_array(seed, max_test_length, 4, 8, 10, 100));
	ret.n_outputs = ret.inputs;

	for_each_subsequence_of(ret.n_outputs[0], 0, [&](auto begin, auto end) {
		std::ranges::sort(begin, end);
	});
	return ret;
}
static maybe_test random_test_stored_image_decoder(uint32_t seed) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_unknown(uint32_t seed) {
	single_test ret{};
	xorshift128_engine engine(seed);
	ret.inputs.push_back(zero_vec());
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
	return ret;
}
static maybe_test random_test_sequence_merger(uint32_t seed) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_integer_series_calculator(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1, zero_vec());
	ret.n_outputs.resize(1, zero_vec());
	for (std::size_t i = 0; i < max_test_length; i++) {
		word_t n = engine.next_word(1, 44);
		ret.inputs[0][i] = n;
		ret.n_outputs[0][i] = to_word(n * (n + 1) / 2);
	}
	return ret;
}
static maybe_test random_test_sequence_range_limiter(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs = {zero_vec(6), zero_vec(0), zero_vec(6)};
	word_vec& mininput = ret.inputs[0];
	word_vec& input = ret.inputs[1];
	word_vec& maxinput = ret.inputs[2];
	ret.n_outputs.resize(1);
	word_vec& output = ret.n_outputs[0];
	for (std::size_t i = 0; i < 6; i++) {
		mininput[i] = engine.next_word(3, 9) * 5;
	}
	for (std::size_t i = 0; i < 6; i++) {
		maxinput[i] = engine.next_word(10, 17) * 5;
	}
	for (std::size_t i = 0; i < 6; i++) {
		for (std::size_t j = 0; j < 5; j++) {
			word_t val = engine.next_word(10, 99);
			input.push_back(val);
			output.push_back(std::clamp(val, mininput[i], maxinput[i]));
		}
		input.push_back(0);
		output.push_back(0);
	}
	return ret;
}
static maybe_test random_test_signal_error_corrector(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(2, zero_vec());
	ret.n_outputs.resize(2, zero_vec());
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
	return ret;
}
static maybe_test random_test_subsequence_extractor(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(2);
	ret.n_outputs.resize(1);
	word_vec& in_indexes = ret.inputs[0];
	word_vec& in_seq = ret.inputs[1];
	word_vec& out = ret.n_outputs[0];

	std::array<word_t, 8> seq_lengths = {2, 3, 3, 4, 4, 4, 5, 6};
	for (std::size_t i = seq_lengths.size() - 1; i >= 1; i--) {
		std::size_t j = engine.next_word(0, to_word(i));
		std::swap(seq_lengths[i], seq_lengths[j]);
	}

	for (word_t len : seq_lengths) {
		for (word_t j = 0; j < len; j++) {
			in_seq.push_back(engine.next_word(10, 99));
		}
		in_seq.push_back(0);
		word_t sublen = engine.next_word(2, len);
		word_t first = engine.next_word(0, len - sublen);
		word_t last = to_word(first + sublen - 1);
		in_indexes.push_back(first);
		in_indexes.push_back(last);
		auto in_it = in_seq.end() - len - 1 + first;
		out.insert(out.end(), in_it, in_it + sublen);
		out.push_back(0);
	}
	return ret;
}
static maybe_test random_test_signal_prescaler(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1, zero_vec());
	ret.n_outputs.resize(3, zero_vec());
	for (int i = 0; i < max_test_length; i++) {
		word_t val = engine.next_word(1, 120);
		ret.n_outputs[2][i] = val;
		ret.n_outputs[1][i] = val * 2;
		ret.n_outputs[0][i] = val * 4;
		ret.inputs[0][i] = val * 8;
	}
	return ret;
}
static maybe_test random_test_signal_averager(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(2, zero_vec());
	ret.n_outputs.resize(1, zero_vec());
	for (int i = 0; i < max_test_length; i++) {
		word_t valA = engine.next_word(100, 999);
		word_t valB = engine.next_word(100, 999);
		ret.inputs[0][i] = valA;
		ret.inputs[1][i] = valB;
		ret.n_outputs[0][i] = to_word((valA + valB) / 2);
	}
	return ret;
}
static maybe_test random_test_submaximum_selector(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(4, zero_vec());
	ret.n_outputs.resize(1, zero_vec());
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
	return ret;
}
static maybe_test random_test_decimal_decomposer(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1, zero_vec());
	ret.n_outputs.resize(3, zero_vec());
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
	return ret;
}
static maybe_test random_test_sequence_mode_calculator(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1, zero_vec());
	ret.n_outputs.resize(1);

	int last_zero = -1;
	for (int i = 0; i < max_test_length - 1; i++) {
		ret.inputs[0][i] = engine.next_word(1, 5);
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
			uint max_frequency = 0;
			uint most_frequent = 0;
			for (uint k = 0; k < 5; k++) {
				if (frequency[k] > max_frequency) {
					most_frequent = k + 1;
					max_frequency = frequency[k];
				} else if (frequency[k] == max_frequency) {
					most_frequent = 0;
				}
			}
			ret.n_outputs[0].push_back(to_word(most_frequent));
			frequency.fill(0u);
		} else {
			frequency[input - 1]++;
		}
	}
	return ret;
}
static maybe_test random_test_sequence_normalizer(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1, zero_vec(max_test_length - 1));
	ret.n_outputs.resize(1, zero_vec(max_test_length - 1));

	int curr_start = 0;
	for (int i = 0; i < max_test_length - 1; i++) {
		word_t val = engine.next_word(1, 99);
		ret.inputs[0][i] = val;
		ret.n_outputs[0][i] = val;

		auto cur_seq
		    = std::span(ret.n_outputs[0]).subspan(curr_start, i - curr_start + 1);
		if ((engine.next_word(1, 3) == 3 and cur_seq.size() > 2)
		    or (cur_seq.size() > 7) or (i == max_test_length - 3)) {
			word_t min_in_seq = std::ranges::min(cur_seq);
			for (word_t& seqval : cur_seq) {
				seqval -= min_in_seq;
			}

			i++;
			ret.inputs[0][i] = -1;
			ret.n_outputs[0][i] = -1;
			curr_start = i + 1;
		}
	}
	ret.n_outputs[0].resize(curr_start);
	return ret;
}
static maybe_test random_test_image_test_pattern_3(uint32_t) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_image_test_pattern_4(uint32_t) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_spatial_path_viewer(uint32_t seed) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_character_terminal(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1);
	ret.i_outputs.emplace_back(image_width, image_height);

	bool char_decode[][2][2] = {{{0, 0}, {0, 0}},
	                            {{1, 1}, {0, 0}},
	                            {{1, 0}, {0, 1}},
	                            {{0, 1}, {1, 0}},
	                            {{1, 1}, {1, 0}}};

	auto render_character = [&](word_t x, word_t y, word_t c) {
		auto ch = char_decode[c];
		for (int a : {0, 1}) {
			for (int b : {0, 1}) {
				if (ch[a][b]) {
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

	input[engine.next_word(12, 16)] = 0;
	input[engine.next_word(28, 31)] = 0;

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
	return ret;
}
static maybe_test random_test_back_reference_reifier(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(2, zero_vec());
	ret.n_outputs.resize(1, zero_vec());
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
	return ret;
}
static maybe_test random_test_dynamic_pattern_detector(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs = {word_vec(4), word_vec(max_test_length)};
	ret.n_outputs.push_back(word_vec(max_test_length));
	auto& pattern = ret.inputs[0];
	auto& input = ret.inputs[1];
	auto& output = ret.n_outputs[0];

	for (int i = 0; i < 12; i++) {
		engine.next_double();
	}

	for (int i = 0; i < 3; i++) {
		pattern[i] = engine.next_word(1, 42);
	}
	pattern.back() = 0;
	for (int i = 0; i < max_test_length; i++) {
		input[i] = engine.next_word(1, 42);
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
	return ret;
}
static maybe_test random_test_sequence_gap_interpolator(uint32_t seed) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_decimal_to_octal_converter(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1, zero_vec());
	ret.n_outputs.resize(1, zero_vec());
	auto to_octal = [](word_t i) { return (i / 8) * 10 + (i % 8); };

	for (auto i : range(max_test_length)) {
		auto v = ret.inputs[0][i] = engine.next_word(1, 63);
		ret.n_outputs[0][i] = to_word(to_octal(v));
	}
	return ret;
}
static maybe_test random_test_prolonged_sequence_sorter(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1, zero_vec());

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
	return ret;
}
static maybe_test random_test_prime_factor_calculator(uint32_t seed) {
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

	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(1, zero_vec(10));
	ret.n_outputs.resize(1, zero_vec(max_test_length - 1));
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
		++it; // 0
	}
	return ret;
}
static maybe_test random_test_signal_exponentiator(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(2, zero_vec());
	ret.n_outputs.resize(1, zero_vec());
	// extra 0 at the beginning because Lua arrays start at 1
	std::array<word_t, 11> max_exp{0, 10, 9, 6, 4, 4, 3, 3, 3, 3, 2};

	for (auto i : range(max_test_length)) {
		auto a = ret.inputs[0][i] = engine.next_word(1, 10);
		auto b = ret.inputs[1][i] = engine.next_word(1, max_exp[a]);
		ret.n_outputs[0][i] = to_word(std::pow(a, b));
	}
	return ret;
}
static maybe_test random_test_t20_node_emulator(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs = {zero_vec(), word_vec()};
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
	clamp_test_values(ret);
	return ret;
}
static maybe_test random_test_t31_node_emulator(uint32_t seed) {
	single_test ret{};
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
	return ret;
}
static maybe_test random_test_wave_collapse_supervisor(uint32_t seed) {
	single_test ret{};
	lua_random engine(to_signed(seed));
	ret.inputs.resize(4, zero_vec());
	ret.n_outputs.resize(1, zero_vec());
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
	return ret;
}

// clang-format off

using enum node_type_t;
constexpr std::array<builtin_level, builtin_levels_num> builtin_levels = {{
	{"00150", "SELF-TEST DIAGNOSTIC", 50, {
		.nodes = {{
			{T21, Damaged, T21, T21, },
			{T21, Damaged, T21, Damaged, },
			{T21, Damaged, T21, T21, },
		}},
		.inputs  = {{in, null, null, in, }},
		.outputs = {{out, null, null, out, }},
	}, &random_test_self_test_diagnostic },
	{"10981", "SIGNAL AMPLIFIER", 2, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_signal_amplifier },
	{"20176", "DIFFERENTIAL CONVERTER", 3, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, out, out, null, }},
	}, &random_test_differential_converter },
	{"21340", "SIGNAL COMPARATOR", 4, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, Damaged, Damaged, Damaged, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{in, null, null, null, }},
		.outputs = {{null, out, out, out, }},
	}, &random_test_signal_comparator },
	{"22280", "SIGNAL MULTIPLEXER", 22, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, in, in, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_signal_multiplexer },
	{"30647", "SEQUENCE GENERATOR", 5, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, Damaged, T21, T21, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_sequence_generator },
	{"31904", "SEQUENCE COUNTER", 9, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, out, out, null, }},
	}, &random_test_sequence_counter },
	{"32050", "SIGNAL EDGE DETECTOR", 7, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_signal_edge_detector },
	{"33762", "INTERRUPT HANDLER", 19, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{in, in, in, in, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_interrupt_handler },
	{"USEG0", "SIMPLE SANDBOX", 1, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_simple_sandbox },
	{"40196", "SIGNAL PATTERN DETECTOR", 888, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_signal_pattern_detector },
	{"41427", "SEQUENCE PEAK DETECTOR", 18, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, out, out, null, }},
	}, &random_test_sequence_peak_detector },
	{"42656", "SEQUENCE REVERSER", 10, {
		.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T30, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_sequence_reverser },
	{"43786", "SIGNAL MULTIPLIER", 6, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_signal_multiplier },
	{"USEG1", "STACK MEMORY SANDBOX", 1, {
		.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_stack_memory_sandbox },
	{"50370", "IMAGE TEST PATTERN 1", 13, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, null, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_image_test_pattern_1 },
	{"51781", "IMAGE TEST PATTERN 2", 14, {
		.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, null, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_image_test_pattern_2 },
	{"52544", "EXPOSURE MASK VIEWER", 60, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_exposure_mask_viewer },
	{"53897", "HISTOGRAM VIEWER", 15, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_histogram_viewer },
	{"USEG2", "IMAGE CONSOLE SANDBOX", 1, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_image_console_sandbox },
	{"60099", "SIGNAL WINDOW FILTER", 55, {
		.nodes = {{
			{Damaged, T21, T21, T30, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T30, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, out, out, null, }},
	}, &random_test_signal_window_filter },
	{"61212", "SIGNAL DIVIDER", 16, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, out, out, null, }},
	}, &random_test_signal_divider },
	{"62711", "SEQUENCE INDEXER", 11, {
		.nodes = {{
			{T21, T30, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs  = {{in, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_sequence_indexer },
	{"63534", "SEQUENCE SORTER", 12, {
		.nodes = {{
			{Damaged, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_sequence_sorter },
	{"70601", "STORED IMAGE DECODER", 21, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_stored_image_decoder },
	{"UNKNOWN", "UNKNOWN", 23, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, Damaged, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, out, out, null, }},
	}, &random_test_unknown },
	{"NEXUS.00.526.6", "SEQUENCE MERGER", 0 * 23, {
		.nodes = {{
			{T21, T21, Damaged, T21, },
			{T30, T21, T21, T21, },
			{T21, T21, T21, T30, },
		}},
		.inputs  = {{null, in, null, in, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_sequence_merger },
	{"NEXUS.01.874.8", "INTEGER SERIES CALCULATOR", 1 * 23, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, out, null, null, }},
	}, &random_test_integer_series_calculator },
	{"NEXUS.02.981.2", "SEQUENCE RANGE LIMITER", 2 * 23, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{in, in, in, null, }},
		.outputs = {{null, out, null, null, }},
	}, &random_test_sequence_range_limiter },
	{"NEXUS.03.176.9", "SIGNAL ERROR CORRECTOR", 3 * 23, {
		.nodes = {{
			{Damaged, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, out, out, null, }},
	}, &random_test_signal_error_corrector },
	{"NEXUS.04.340.5", "SUBSEQUENCE EXTRACTOR", 4 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_subsequence_extractor },
	{"NEXUS.05.647.1", "SIGNAL PRESCALER", 5 * 23, {
		.nodes = {{
			{T21, Damaged, Damaged, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{in, null, null, null, }},
		.outputs = {{null, out, out, out, }},
	}, &random_test_signal_prescaler },
	{"NEXUS.06.786.0", "SIGNAL AVERAGER", 6 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_signal_averager },
	{"NEXUS.07.050.0", "SUBMAXIMUM SELECTOR", 7 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{in, in, in, in, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_submaximum_selector },
	{"NEXUS.08.633.9", "DECIMAL DECOMPOSER", 8 * 23, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{out, out, out, null, }},
	}, &random_test_decimal_decomposer },
	{"NEXUS.09.904.9", "SEQUENCE MODE CALCULATOR", 9 * 23, {
		.nodes = {{
			{T30, T21, T30, Damaged, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, out, null, null, }},
	}, &random_test_sequence_mode_calculator },
	{"NEXUS.10.656.5", "SEQUENCE NORMALIZER", 10 * 23, {
		.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T30, },
			{T21, Damaged, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_sequence_normalizer },
	{"NEXUS.11.711.2", "IMAGE TEST PATTERN 3", 11 * 23, {
		.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, null, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_image_test_pattern_3 },
	{"NEXUS.12.534.4", "IMAGE TEST PATTERN 4", 12 * 23, {
		.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, null, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_image_test_pattern_4 },
	{"NEXUS.13.370.9", "SPATIAL PATH VIEWER", 13 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_spatial_path_viewer },
	{"NEXUS.14.781.3", "CHARACTER TERMINAL", 14 * 23, {
		.nodes = {{
			{T30, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T30, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}, &random_test_character_terminal },
	{"NEXUS.15.897.9", "BACK-REFERENCE REIFIER", 15 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_back_reference_reifier },
	{"NEXUS.16.212.8", "DYNAMIC PATTERN DETECTOR", 16 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs  = {{in, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_dynamic_pattern_detector },
	{"NEXUS.17.135.0", "SEQUENCE GAP INTERPOLATOR", 17 * 23, {
		.nodes = {{
			{Damaged, T21, T21, T21, },
			{Damaged, T30, T21, T30, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_sequence_gap_interpolator },
	{"NEXUS.18.427.7", "DECIMAL TO OCTAL CONVERTER", 18 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_decimal_to_octal_converter },
	{"NEXUS.19.762.9", "PROLONGED SEQUENCE SORTER", 19 * 23, {
		.nodes = {{
			{Damaged, T30, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs  = {{null, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_prolonged_sequence_sorter },
	{"NEXUS.20.433.1", "PRIME FACTOR CALCULATOR", 20 * 23, {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{null, in, null, null, }},
		.outputs = {{null, out, null, null, }},
	}, &random_test_prime_factor_calculator },
	{"NEXUS.21.601.6", "SIGNAL EXPONENTIATOR", 21 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_signal_exponentiator },
	{"NEXUS.22.280.8", "T20 NODE EMULATOR", 22 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs  = {{null, in, in, null, }},
		.outputs = {{null, out, null, null, }},
	}, &random_test_t20_node_emulator },
	{"NEXUS.23.727.9", "T31 NODE EMULATOR", 23 * 23, {
		.nodes = {{
			{Damaged, T30, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs  = {{null, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}, &random_test_t31_node_emulator },
	{"NEXUS.24.511.7", "WAVE COLLAPSE SUPERVISOR", 24 * 23, {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs  = {{in, in, in, in, }},
		.outputs = {{null, out, null, null, }},
	}, &random_test_wave_collapse_supervisor },
}};
