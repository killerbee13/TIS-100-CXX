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
#ifndef TIS_RANDOM_HPP
#define TIS_RANDOM_HPP

#include "node.hpp"

using uint = std::uint32_t;

class xorshift128_engine {
 public:
	uint x = 0, y = 0, z = 0, w = 0;

 private:
	constexpr static uint MT19937 = 1812433253;

 public:
	constexpr explicit xorshift128_engine(uint seed) noexcept
	    : x(seed)
	    , y(MT19937 * x + 1)
	    , z(MT19937 * y + 1)
	    , w(MT19937 * z + 1) {}
	constexpr xorshift128_engine(uint x, uint y, uint z, uint w) noexcept
	    : x(x)
	    , y(y)
	    , z(z)
	    , w(w) {}

	constexpr uint next() noexcept {
		uint t = x ^ (x << 11);
		x = y;
		y = z;
		z = w;
		return w = w ^ (w >> 19) ^ t ^ (t >> 8);
	}
	constexpr uint next(uint min, uint max) noexcept {
		if (max - min == 0)
			return min;

		if (max < min)
			return min - next() % (max + min);
		else
			return min + next() % (max - min);
	}
	constexpr word_t next_int(word_t min, word_t max) noexcept {
		if (max == min) {
			return min;
		}

		std::int64_t minLong = static_cast<std::int64_t>(min);
		std::int64_t maxLong = static_cast<std::int64_t>(max);
		std::int64_t r = next();

		if (max < min)
			return static_cast<word_t>(minLong - r % (maxLong - minLong));
		else
			return static_cast<word_t>(minLong + r % (maxLong - minLong));
	}
};

#endif // TIS_RANDOM_HPP
