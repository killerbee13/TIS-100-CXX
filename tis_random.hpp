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
#ifndef TIS_RANDOM_HPP
#define TIS_RANDOM_HPP

#include "utils.hpp"
#include <cstdint>
using u32 = std::uint32_t;
using i32 = std::int32_t;

class xorshift128_engine {
 public:
	u32 x = 0, y = 0, z = 0, w = 0;

 private:
	constexpr static u32 MT19937 = 1812433253;

 public:
	constexpr explicit xorshift128_engine(u32 seed) noexcept
	    : x(seed)
	    , y(MT19937 * x + 1)
	    , z(MT19937 * y + 1)
	    , w(MT19937 * z + 1) {}
	constexpr xorshift128_engine(u32 x, u32 y, u32 z, u32 w) noexcept
	    : x(x)
	    , y(y)
	    , z(z)
	    , w(w) {}

	constexpr u32 next() noexcept {
		u32 t = x ^ (x << 11);
		x = y;
		y = z;
		z = w;
		return w = w ^ (w >> 19) ^ t ^ (t >> 8);
	}
	constexpr u32 next(u32 min, u32 max) noexcept {
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

// based on
// https://github.com/microsoft/referencesource/blob/master/mscorlib/system/random.cs
// but with inextp = 31 instead of 21 and the min+1==max escape hatch, matching
// the old mono version in the game
class lua_random {
 private:
	i32 inext{};
	i32 inextp{31};
	std::array<i32, 56> seed_array{};

	i32 map_negative(i32 x) {
		if (x < 0) {
			x += kblib::max.of<i32>();
		}
		return x;
	}

 public:
	lua_random(i32 random_seed) {
		assert(inextp >= 0 and to_unsigned(inextp) < seed_array.size());

		i32 subtraction
		    = (random_seed == kblib::min) ? kblib::max : std::abs(random_seed);
		i32 mj = 161803398 - subtraction;
		seed_array.back() = mj;
		i32 mk = 1;
		std::size_t ii;
		for (const auto i : range(1u, 55u)) {
			ii = (21u * i) % 55u;
			seed_array[ii] = mk;
			mk = map_negative(mj - mk);
			mj = seed_array[ii];
		}
		for (int k = 1; k < 5; ++k) {
			for (const auto i : range(1u, 56u)) {
				// Do the subtraction in unsigned because it can overflow
				seed_array[i] = map_negative(
				    to_signed(to_unsigned(seed_array[i])
				              - to_unsigned(seed_array[1u + (i + 30u) % 55u])));
			}
		}
	}

	double next_double() {
		if (++inext >= 56) {
			inext = 1;
		}
		if (++inextp >= 56) {
			inextp = 1;
		}

		i32 ret
		    = seed_array[to_unsigned(inext)] - seed_array[to_unsigned(inextp)];

		if (ret == kblib::max) {
			--ret;
		}
		if (ret < 0) {
			ret += kblib::max.of<i32>();
		}
		seed_array[to_unsigned(inext)] = ret;

		return ret * (1.0 / kblib::max.of<i32>());
	}

	i32 next_int(i32 min, i32 max) {
		assert(min < max); // all our calls are static, no need to throw
		if (max == min + 1) {
			return min;
		}
		return static_cast<i32>(next_double() * (max - min)) + min;
	}

	/// @return random word_t in [min,max]
	word_t next(word_t min, word_t max) {
		return static_cast<word_t>(
		    next_int(static_cast<i32>(min), static_cast<i32>(max) + 1));
	}
};

#endif // TIS_RANDOM_HPP
