/* *****************************************************************************
 * TIX-100-CXX
 * Copyright (c) 2024 killerbee
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
#ifndef NODE_HPP
#define NODE_HPP

#include <array>
#include <cstdint>

using word_t = std::int16_t;
using index_t = std::int8_t;

enum port : std::int8_t {
	up,
	left,
	right,
	down,
	D5, // 3D expansion
	D6,
	nil_,
	acc,
	any_,
	last,
	immediate = -1
};

struct node {
 public:
	enum type_t { T21, T30, in, out, image, Damaged = -1 };

	virtual type_t type() const = 0;
	virtual void step() = 0;
	virtual void read() = 0;
	std::array<node*, 6> neighbors;
	int x{};
	int y{};

	node() = default;
	virtual ~node() = default;

 protected: // prevents most slicing
	node(const node&) = default;
	node(node&&) = default;
	node& operator=(const node&) = default;
	node& operator=(node&&) = default;
};

struct damaged : node {
	type_t type() const override { return Damaged; }
	void step() override { return; }
	void read() override { return; }
};

#endif // NODE_HPP
