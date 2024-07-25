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

inline std::vector<word_t> make_random_array(std::uint32_t seed,
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
inline std::vector<word_t> make_random_array(xorshift128_engine& engine,
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

inline std::vector<word_t> make_composite_array(uint seed, word_t size,
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
inline std::vector<word_t> make_composite_array(xorshift128_engine& engine,
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

#endif // TIS_RANDOM_HPP
