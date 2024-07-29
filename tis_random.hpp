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

class lua_random {
 private:
	int32_t inext{};
	int32_t inextp{};
	std::array<int32_t, 56> seed_array{};

	int32_t map_negative(int32_t x) {
		if (x < 0) {
			x += kblib::max.of<int32_t>();
		}
		return x;
	}

 protected:
	lua_random(int32_t random_seed, int32_t initial_inextp)
	    : inext{}
	    , inextp{initial_inextp} {
		assert(initial_inextp >= 0
		       and kblib::to_unsigned(initial_inextp) < seed_array.size());

		int32_t subtraction
		    = (random_seed == kblib::min) ? kblib::max : std::abs(random_seed);
		int32_t mj = 161803398 - subtraction;
		seed_array.back() = mj;
		int32_t mk = 1;
		std::size_t ii;
		for (const auto i : kblib::range(1u, 55u)) {
			ii = (21u * i) % 55u;
			seed_array[ii] = mk;
			mk = map_negative(mj - mk);
			mj = seed_array[ii];
		}
		for (int k = 1; k < 5; ++k) {
			for (const auto i : kblib::range(1u, 56u)) {
				// Do the subtraction in unsigned because it can overflow
				seed_array[i] = map_negative(kblib::to_signed(
				    kblib::to_unsigned(seed_array[i])
				    - kblib::to_unsigned(seed_array[1u + (i + 30u) % 55u])));
			}
		}
	}

 public:
	lua_random(int32_t seed)
	    : lua_random(seed, 31) {}

	int32_t next(int32_t max) {
		if (max <= 1) {
			return 0;
		}

		if (++inext >= 56) {
			inext = 1;
		}
		if (++inextp >= 56) {
			inextp = 1;
		}

		int32_t ret = seed_array[kblib::to_unsigned(inext)]
		              - seed_array[kblib::to_unsigned(inextp)];

		if (ret == kblib::max) {
			--ret;
		}
		if (ret < 0) {
			ret += kblib::max.of<int32_t>();
		}
		seed_array[kblib::to_unsigned(inext)] = ret;

		return static_cast<int32_t>(ret * (1.0 / kblib::max.of<int32_t>()) * max);
	}
	word_t next(word_t max) {
		return static_cast<word_t>(next(static_cast<int32_t>(max) + 1));
	}
};

#endif // TIS_RANDOM_HPP
