/* *****************************************************************************
 * TIX-100-CXX
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
#ifndef NODE_HPP
#define NODE_HPP

#include "logger.hpp"
#include "utils.hpp"

#include <array>

/// Support a 3D expansion in node connections
constexpr inline int DIMENSIONS = 2;

constexpr inline int def_T21_size = 15;
constexpr inline int def_T30_size = 15;

enum class activity : std::int8_t { idle, run, read, write };

constexpr static std::string_view state_name(activity s) {
	switch (s) {
	case activity::idle:
		return "IDLE";
	case activity::run:
		return "RUN";
	case activity::read:
		return "READ";
	case activity::write:
		return "WRTE";
	default:
		throw std::invalid_argument{""};
	}
}

enum port : std::int8_t {
	left,
	right,
	up,
	down,
	D5, // 3D expansion
	D6,

	// The directions are values 0-N, so < and > can be used to compare them
	dir_first = 0,
	dir_last = DIMENSIONS * 2 - 1,

	nil = std::max(D6, dir_last) + 1,
	acc,
	any,
	last,
	immediate = -1
};

constexpr port& operator++(port& p) {
	assert(p >= port::dir_first and p <= port::dir_last);
	p = static_cast<port>(std::to_underlying(p) + 1);
	return p;
}
constexpr port operator++(port& p, int) {
	port old = p;
	++p;
	return old;
}

constexpr port invert(port p) {
	assert(p >= port::dir_first and p <= port::dir_last);
	return static_cast<port>(std::to_underlying(p) ^ 1);
}

constexpr static std::string_view port_name(port p) {
	switch (p) {
	case left:
		return "LEFT";
	case right:
		return "RIGHT";
	case up:
		return "UP";
	case down:
		return "DOWN";
	case D5:
		return "D5";
	case D6:
		return "D6";
	case nil:
		return "NIL";
	case acc:
		return "ACC";
	case any:
		return "ANY";
	case last:
		return "LAST";
	case immediate:
		return "VAL";
	default:
		throw std::invalid_argument{concat("Illegal port ", etoi(p))};
	}
}

struct node {
 public:
	enum type_t : int8_t {
		T21 = 1,
		T30,
		in,
		out,
		image,
		Damaged = -1,
		null = -2
	};

	/// Returns the type of the node
	virtual type_t type() const noexcept = 0;
	/// Attempt to answer a read from this node, coming from direction p
	virtual optional_word emit(port p) = 0;
	/// Generate a string representation of the current state of the node
	virtual std::string state() const = 0;

	int x{};
	int y{};

	node() = default;
	node(int x, int y) noexcept
	    : x(x)
	    , y(y) {}
	virtual ~node() = default;

 protected: // prevents most slicing
	node(const node&) = default;
	node(node&&) = default;
	node& operator=(const node&) = default;
	node& operator=(node&&) = default;
};

struct regular_node : node {
	using node::node;
	/// Begin processing a cycle. Do not perform writes.
	virtual void step(logger& debug) = 0;
	/// Finish processing a cycle. Writes are completed here.
	virtual void finalize(logger& debug) = 0;
	/// Return a new node initialized in the same way as *this.
	/// (Not a copy constructor; new node is as if reset() and has no neighbors)
	virtual std::unique_ptr<regular_node> clone() const = 0;

	/// only useful nodes are linked, other links from-to are nullptr
	std::array<node*, 2 * DIMENSIONS> neighbors{};

 protected:
	/// Attempt to read a value from p, coming from this node
	[[gnu::always_inline]] inline optional_word do_read(port p) {
		assert(p >= port::dir_first and p <= port::dir_last);
		node* n = neighbors[to_unsigned(etoi(p))];
		if (not n) {
			return word_empty;
		} else {
			return n->emit(invert(p));
		}
	}
};

struct damaged final : regular_node {
	using regular_node::regular_node;
	type_t type() const noexcept override { return Damaged; }
	void step(logger&) override {}
	void finalize(logger&) override {}
	std::unique_ptr<regular_node> clone() const override {
		return std::make_unique<damaged>(x, y);
	}
	optional_word emit(port) override { return word_empty; }
	std::string state() const override {
		return concat("(", x, ',', y, ") {Damaged}");
	}
};

struct hcf_exception {
	int x{};
	int y{};
	int line{};
};

#endif // NODE_HPP
