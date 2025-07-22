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

#include "tis100.hpp"
#include "utils.hpp"

#include <array>

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
		throw std::invalid_argument{concat("Invalid activity (", etoi(s), ")")};
	}
}

struct node {
 public:
	using type_t = node_type_t;
	using enum node_type_t;

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
	node(const node&) = delete;
	node(node&&) = delete;
	node& operator=(const node&) = delete;
	node& operator=(node&&) = delete;
};

constexpr std::string to_string(node::type_t type) {
	switch (type) {
	case node::T21:
		return "T21";
	case node::T30:
		return "T30";
	case node::in:
		return "input";
	case node::out:
		return "num_out";
	case node::image:
		return "image";
	case node::Damaged:
		return "damaged";
	case node::null:
		return "type_t::null";
	default:
		throw std::invalid_argument(concat("Invalid type_t(", etoi(type), ")"));
	}
}

struct regular_node : node {
	virtual ~regular_node() = default;

	/// Generate a string representation of the current state of the node
	virtual std::string state() const = 0;
	/// Return a new node initialized in the same way as *this.
	/// (Not a copy constructor; new node is as if reset() and has no neighbors)
	virtual std::unique_ptr<regular_node> clone() const = 0;
	/// Reset for new test
	virtual void reset() noexcept = 0;

	/// only useful nodes are linked, other links from-to are nullptr
	std::array<node*, 2 * DIMENSIONS> neighbors{};

 protected:
	using node::node;
	/// Attempt to read a value from p, coming from this node
	[[gnu::always_inline]] inline optional_word do_read(port p) const {
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
	void reset() noexcept override {}
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
