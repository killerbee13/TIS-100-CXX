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

	int x{};
	int y{};

	/// word the node wants to write
	optional_word write_word = word_empty;
	/// The direction we are writing into, acts as a semaphore of sort.
	/// The reader sets it to indicate it has read the word to nil (normally) or
	/// the actual read direction (if it was `any`).
	/// The special `any` handling is needed because T21 needs the acual
	/// read direction to set `last`.
	port write_port = port::nil;
	/// the type of this node, never actually null
	type_t type = type_t::null;

	/// Attempt to answer a read from this node, coming from direction p
	[[gnu::always_inline]] inline optional_word emit(port p) {
		if (write_word != word_empty
		    && (write_port == p or write_port == port::any)) {
			if (write_port == port::any) {
				// we pass the actual read port back for the T21 to use it
				write_port = p;
			} else {
				// we send a "we have read" message in any case, because nodes
				// can't efficiently manage I/O with just the `write_word`,
				// so it acts as a semaphore
				write_port = port::nil;
			}
			return std::exchange(write_word, word_empty);
		} else {
			return word_empty;
		}
	}

 protected:
	// effectively abstract
	node() = delete;
	~node() = default;
	node(int x, int y, type_t type) noexcept
	    : x(x)
	    , y(y)
	    , type(type) {}
	// prevents most slicing
	node(const node&) = default;
	node(node&&) = default;
	node& operator=(const node&) = default;
	node& operator=(node&&) = default;
};

struct regular_node : node {
	virtual ~regular_node() = default;

	/// Generate a string representation of the current state of the node
	virtual std::string state() const = 0;
	/// Return a new node initialized in the same way as *this.
	/// (Not a copy constructor; new node is as if reset() and has no neighbors)
	virtual std::unique_ptr<regular_node> clone() const = 0;

	/// only useful nodes are linked, other links from-to are nullptr
	std::array<node*, 2 * DIMENSIONS> neighbors{};

 protected:
	using node::node;
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
	damaged(int x, int y) noexcept
	    : regular_node(x, y, type_t::Damaged) {}
	std::unique_ptr<regular_node> clone() const override {
		return std::make_unique<damaged>(x, y);
	}
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
