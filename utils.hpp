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

#endif // UTILS_HPP
