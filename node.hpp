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
	// The directions are values 0-5, so < and > can be used to compare them
	left,
	right,
	up,
	down,
	D5, // 3D expansion
	D6,

	nil,
	acc,
	any,
	last,
	immediate = -1
};

constexpr port invert(port p) {
	switch (p) {
	case left:
		return right;
	case right:
		return left;
	case up:
		return down;
	case down:
		return up;
	case D5:
		return D6;
	case D6:
		return D5;
	default:
		std::unreachable();
	}
}

struct node {
 public:
	enum type_t { T21 = 1, T30, in, out, image, Damaged = -1, null = -2 };

	constexpr static std::string_view port_name(port p) {
		switch (p) {
		case up:
			return "UP";
		case left:
			return "LEFT";
		case right:
			return "RIGHT";
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

	/// Returns the type of the node
	virtual type_t type() const noexcept = 0;
	/// Begin processing a cycle. Do not perform writes.
	virtual void step(logger& debug) = 0;
	/// Finish processing a cycle. Writes are completed here.
	virtual void finalize(logger& debug) = 0;
	/// Return a new node initialized in the same way as *this.
	/// (Not a copy constructor; new node is as if reset() and has no neighbors)
	virtual std::unique_ptr<node> clone() const = 0;

	/// Attempt to answer a read from this node, coming from direction p
	virtual optional_word emit(port p) = 0;
	/// Generate a string representation of the current state of the node
	virtual std::string state() const = 0;

	// In theory this may be expanded to a "TIS-3D" simulator which will have 6
	// neighbors for each node.
	std::array<node*, 6> neighbors{};
	int x{};
	int y{};

	node() = default;
	node(int x, int y) noexcept
	    : x(x)
	    , y(y) {}
	virtual ~node() = default;

	/// null and Damaged are negative. Any positive value is a valid node
	// (0 is unallocated)
	friend bool valid(const node* n) { return n and etoi(n->type()) > 0; }
	/// Attempt to read a value from n, coming from direction p
	friend optional_word do_read(node* n, port p) {
		assert(p >= port::left and p <= port::D6);
		if (not n) {
			return word_empty;
		} else {
			return n->emit(p);
		}
	}

 protected: // prevents most slicing
	node(const node&) = default;
	node(node&&) = default;
	node& operator=(const node&) = default;
	node& operator=(node&&) = default;
};

struct damaged : node {
	using node::node;
	type_t type() const noexcept override { return Damaged; }
	void step(logger&) override {}
	void finalize(logger&) override {}
	std::unique_ptr<node> clone() const override {
		return std::make_unique<damaged>(x, y);
	}
	optional_word emit(port) override { return word_empty; }
	std::string state() const override {
		return concat("(", x, ',', y, ") {Damaged}");
	}
};

template <typename T, typename U>
constexpr U sat_add(T a, T b, U l, U h) {
	using I = std::common_type_t<T, U>;
	return static_cast<U>(
	    std::clamp(static_cast<I>(a + b), static_cast<I>(l), static_cast<I>(h)));
}

template <auto l = word_min, auto h = word_max, typename T>
constexpr auto sat_add(T a, T b) {
	return sat_add<T, std::common_type_t<decltype(l), decltype(h)>>(a, b, l, h);
}
template <auto l = word_min, auto h = word_max, typename T>
constexpr auto sat_sub(T a, T b) {
	return sat_add<T, std::common_type_t<decltype(l), decltype(h)>>(a, -b, l, h);
}

// returns a new string, padded
inline std::string pad_right(std::string input, std::size_t size,
                             char padding = ' ') {
	input.resize(std::max(input.size(), size), padding);
	return input;
}
inline std::string pad_right(std::string_view input, std::size_t size,
                             char padding = ' ') {
	return pad_right(std::string(input), size, padding);
}
inline std::string pad_right(std::integral auto input, std::size_t size,
                             char padding = ' ') {
	return pad_right(std::to_string(input), size, padding);
}

inline std::string pad_left(std::string input, std::size_t size,
                            char padding = ' ') {
	input.insert(0, std::max(0uz, size - input.size()), padding);
	return input;
}
inline std::string pad_left(std::string_view input, std::size_t size,
                            char padding = ' ') {
	return pad_left(std::string(input), size, padding);
}
inline std::string pad_left(std::integral auto input, std::size_t size,
                            char padding = ' ') {
	return pad_left(std::to_string(input), size, padding);
}

template <typename Stream>
   requires requires(Stream s) {
	   { s << "" << 1 };
   }
inline Stream&& write_list(Stream&& os, const word_vec& v,
                           const word_vec* expected = nullptr,
                           bool colored = color_logs) {
	if (colored and expected and v.size() != expected->size()) {
		os << print_escape(bright_red);
	}
	os << '(';
	os << v.size();
	if (expected) {
		os << '/' << expected->size();
	}
	os << ')';
	if (colored) {
		os << print_escape(none);
	}
	os << " [\n\t";
	for (std::size_t i = 0; i < v.size(); ++i) {
		if (colored and expected
		    and (i >= expected->size() or v[i] != (*expected)[i])) {
			os << print_escape(bright_red);
		}
		os << v[i] << ' ';
		if (colored) {
			os << print_escape(none);
		}
	}
	os << "\n]";
	return std::forward<Stream>(os);
}

struct hcf_exception {
	int x{};
	int y{};
	int line{};
};

#endif // NODE_HPP
