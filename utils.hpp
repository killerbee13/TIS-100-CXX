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
#ifndef UTILS_HPP
#define UTILS_HPP

#include <kblib/convert.h>
#include <kblib/stats.h>

#include <cstdint>
#include <vector>

#if NDEBUG
#	define RELEASE 1
#else
#	define RELEASE 0
#endif

using kblib::append, kblib::concat, kblib::etoi, kblib::range, kblib::to_signed,
    kblib::to_unsigned;

using word_t = std::int16_t;
constexpr inline word_t word_min = -999;
constexpr inline word_t word_max = 999;
static_assert(word_min == -word_max);

/// we don't use a whole 16bit for a word, roll out a faster optional<word_t>
using optional_word = word_t;
constexpr inline optional_word word_empty = kblib::min;
static_assert(word_empty < word_min);
static_assert(word_empty < word_min + word_min);

using word_vec = std::vector<word_t>;

template <typename T, typename U>
constexpr U sat_add(T a, T b, U l, U h) {
	using I = std::common_type_t<T, U>;
	return static_cast<U>(
	    std::clamp(static_cast<I>(a + b), static_cast<I>(l), static_cast<I>(h)));
}

template <auto l = word_min, auto h = word_max, typename T>
constexpr auto sat_add(T a, T b) {
	return sat_add<T, std::common_type_t<decltype(l), decltype(h)>>(a, b, l, h);
}
template <auto l = word_min, auto h = word_max, typename T>
constexpr auto sat_sub(T a, T b) {
	return sat_add<T, std::common_type_t<decltype(l), decltype(h)>>(a, -b, l, h);
}

static_assert(sat_add(word_max, word_max) == word_max);
static_assert(sat_add(word_min, word_max) == 0);
static_assert(sat_add(word_min, word_min) == word_min);

// returns a new string, padded
inline std::string pad_right(std::string input, std::size_t size,
                             char padding = ' ') {
	input.resize(std::max(input.size(), size), padding);
	return input;
}
inline std::string pad_right(std::string_view input, std::size_t size,
                             char padding = ' ') {
	return pad_right(std::string(input), size, padding);
}
inline std::string pad_right(std::integral auto input, std::size_t size,
                             char padding = ' ') {
	return pad_right(std::to_string(input), size, padding);
}

inline std::string pad_left(std::string input, std::size_t size,
                            char padding = ' ') {
	input.insert(0, std::max(0uz, size - input.size()), padding);
	return input;
}
inline std::string pad_left(std::string_view input, std::size_t size,
                            char padding = ' ') {
	return pad_left(std::string(input), size, padding);
}
inline std::string pad_left(std::integral auto input, std::size_t size,
                            char padding = ' ') {
	return pad_left(std::to_string(input), size, padding);
}

inline bool color_stdout{false};
inline bool color_logs{false};

enum SGR_code {
	none,
	bold = 1,
	underline = 4,
	reverse = 7,
	normal = 22,
	black = 30,
	red,
	green,
	yellow,
	blue,
	magenta,
	cyan,
	white,
	reset_color = 39,
	bg_black = 40,
	bg_red,
	bg_green,
	bg_yellow,
	bg_blue,
	bg_magenta,
	bg_cyan,
	bg_white,
	reset_bg_color = 49,
	bright_black = 90,
	bright_red,
	bright_green,
	bright_yellow,
	bright_blue,
	bright_magenta,
	bright_cyan,
	bright_white,
	// bright black, also known as grey
	bg_bright_black = 100,
	bg_bright_red,
	bg_bright_green,
	bg_bright_yellow,
	bg_bright_blue,
	bg_bright_magenta,
	bg_bright_cyan,
	bg_bright_white,
};

std::string escape_code(SGR_code first, std::same_as<SGR_code> auto... colors) {
	if (first == none and sizeof...(colors) == 0) {
		return "\033[m";
	}
	std::string ret = "\033["
	                  + (std::to_string(static_cast<int>(first)) + ...
	                     + (';' + std::to_string(static_cast<int>(colors))));
	ret += 'm';
	return ret;
}

std::string print_escape(SGR_code first,
                         std::same_as<SGR_code> auto... colors) {
	if (color_stdout) {
		return escape_code(first, colors...);
	} else {
		return {};
	}
}
std::string log_print_escape(SGR_code first,
                             std::same_as<SGR_code> auto... colors) {
	if (color_logs) {
		return escape_code(first, colors...);
	} else {
		return {};
	}
}

template <typename Stream>
   requires requires(Stream s) {
	   { s << "" << 1 };
	   { s.good() } -> std::same_as<bool>;
   }
inline Stream&& write_list(Stream&& os, const word_vec& v,
                           const word_vec* expected = nullptr,
                           bool colored = color_logs) {
	if (not os.good()) { // this method is costly, immediately bail out
		return std::forward<Stream>(os);
	}
	if (colored and expected and v.size() != expected->size()) {
		os << print_escape(bright_red);
	}
	os << '(';
	os << v.size();
	if (expected) {
		os << '/' << expected->size();
	}
	os << ')';
	if (colored) {
		os << print_escape(none);
	}
	os << " [\n\t";
	for (std::size_t i = 0; i < v.size(); ++i) {
		if (colored and expected
		    and (i >= expected->size() or v[i] != (*expected)[i])) {
			os << print_escape(bright_red);
		}
		os << v[i] << ' ';
		if (colored) {
			os << print_escape(none);
		}
	}
	os << "\n]";
	return std::forward<Stream>(os);
}

#endif // UTILS_HPP
