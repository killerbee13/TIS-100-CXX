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
#ifndef UTILS_HPP
#define UTILS_HPP

#include "logger.hpp"
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
