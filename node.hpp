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

#include <memory>
#include <vector>

struct node {

	enum type_t { T21, Basic = T21, T30, Stack = T30, Damaged = -1 };
	enum port { nil_, acc, left, right, up, down, any_, last };

	type_t type{Damaged};
	std::vector<node*> neighbors;

	node() = default;
	node(const node&) = delete;
	node(node&&) = delete;
	node& operator=(const node&) = delete;
	node& operator=(node&&) = delete;
	virtual ~node() = default;
};

class field {
 public:
 private:
	std::vector<std::unique_ptr<node>> nodes;
};

#endif // NODE_HPP
