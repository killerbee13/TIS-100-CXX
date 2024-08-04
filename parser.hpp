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
#ifndef PARSER_HPP
#define PARSER_HPP

#include "T21.hpp"
#include "io.hpp"
#include "layoutspecs.hpp"
#include "logger.hpp"
#include "node.hpp"

#include <memory>
#include <string>
#include <vector>

class field {
	using data_t = std::vector<std::unique_ptr<node>>;
	using iterator = data_t::iterator;
	using const_iterator = data_t::const_iterator;

 public:
	field() = default;
	field(builtin_layout_spec spec, std::size_t T30_size = def_T30_size);

	/// Advance the field one full cycle (step and finalize)
	void step() {
		// evaluate code
		for (auto& p : nodes) {
			p->step();
		}
		log_debug("");
		// execute writes
		// this is a separate step to ensure a consistent propagation delay
		for (auto& p : nodes) {
			p->finalize();
		}
		log_debug("");
	}

	bool active() const {
		bool active{};
		for (auto it = begin_output(); it != end(); ++it) {
			if ((*it)->type() == node::out) {
				auto i = static_cast<const output_node*>(it->get());
				if (not i->complete) {
					active = true;

// speed up simulator by failing early when an incorrect output is written
#if RELEASE
					if (i->wrong) {
						return false;
					}
#endif
				}
			} else if ((*it)->type() == node::image) {
				auto i = static_cast<const image_output*>(it->get());
				if (i->image_expected != i->image_received) {
					active = true;
				}
			}
		}
		return active;
	}

	/// Write the full state of all nodes, similar to what the game displays in
	/// its debugger but in linear order
	std::string state() const {
		std::string ret;
		for (auto& n : nodes) {
			ret += n->print();
			ret += '\n';
		}
		return ret;
	}

	// Scoring functions
	std::size_t instructions() const;
	std::size_t nodes_used() const;

	// Number of T21 nodes for node_by_index
	std::size_t nodes_avail() const;
	// Number of regular (grid) nodes
	std::size_t nodes_total() const { return in_nodes_offset; }
	// Whether there are any input nodes attached to the field. Image test
	// pattern levels do not use inputs so only need a single test run.
	bool has_inputs() const {
		return begin_io() != begin_output();
	}

	/// Serialize layout as read by parse_layout
	std::string layout() const;
	/// Serialize layout as a C++ builtin_layout_spec initializer
	std::string machine_layout() const;

	// returns the node at the (x,y) coordinates, Nullable
	node* node_by_location(std::size_t x, std::size_t y) {
		if (x > width) {
			return nullptr;
		}
		auto i = y * width + x;
		if (i < nodes.size()) {
			return nodes[i].get();
		} else {
			return nullptr;
		}
	}
	// Nullable
	const node* node_by_location(std::size_t x, std::size_t y) const {
		if (x > width or y > height()) {
			return nullptr;
		}
		auto i = y * width + x;
		if (i < nodes.size()) {
			return nodes[i].get();
		} else {
			return nullptr;
		}
	}
	// returns the ith programmable (T21) node
	T21* node_by_index(std::size_t i) {
		for (auto it = begin(); it != end_regular(); ++it) {
			auto p = it->get();
			if (p->type() == node::T21 and i-- == 0) {
				return static_cast<T21*>(p);
			}
		}
		return nullptr;
	}
	const T21* node_by_index(std::size_t i) const {
		for (auto it = begin(); it != end_regular(); ++it) {
			auto p = it->get();
			if (p->type() == node::T21 and i-- == 0) {
				return static_cast<const T21*>(p);
			}
		}
		return nullptr;
	}

	template <bool use_nonstandard_rep>
	friend field parse_layout(std::string_view layout, std::size_t T30_size);

	const_iterator begin() const noexcept { return nodes.begin(); }
	// Partition between regular and IO nodes
	const_iterator end_regular() const noexcept {
		return nodes.begin() + static_cast<std::ptrdiff_t>(in_nodes_offset);
	}
	const_iterator begin_io() const noexcept { return end_regular(); }
	const_iterator begin_output() const noexcept {
		return nodes.begin() + static_cast<std::ptrdiff_t>(out_nodes_offset);
	}
	const_iterator end() const noexcept { return nodes.end(); }

 private:
	// w*h regulars, 0..w inputs, 1..w outputs
	std::vector<std::unique_ptr<node>> nodes;
	std::size_t width{};
	std::size_t height() const {
		if (in_nodes_offset == 0) {
			return 0;
		}
		return in_nodes_offset / width;
	}
	// must be a multiple of width
	std::size_t in_nodes_offset{};
	// >= in_nodes_offset
	std::size_t out_nodes_offset{};
};

/// Parse a layout string, guessing whether to use BSMC or CSMD
field parse_layout_guess(std::string_view layout, std::size_t T30_size);
/// Assemble a single node's code
std::vector<instr> assemble(std::string_view source, int node,
                            std::size_t T21_size = def_T21_size);
/// Read a TIS-100-compatible save file
void parse_code(field& f, std::string_view source, std::size_t T21_size);
/// Configure the field with a test case
void set_expected(field& f, const single_test& expected);

struct score {
	std::size_t cycles{};
	std::size_t nodes{};
	std::size_t instructions{};
	bool validated{};
	bool achievement{};
	bool cheat{};
	bool zero_random{};

	friend std::ostream& operator<<(std::ostream& os, score sc) {
		if (sc.validated) {
			os << sc.cycles << '/';
		} else {
			os << print_escape(red) << "-/";
		}
		os << sc.nodes << '/' << sc.instructions;
		if (sc.validated) {
			if ((sc.achievement or sc.cheat)) {
				os << '/';
			}
			if (sc.achievement) {
				os << print_escape(bright_blue, bold) << 'a' << print_escape(none);
			}
			if (sc.zero_random) {
				os << print_escape(red) << 'z';
			} else if (sc.cheat) {
				os << print_escape(yellow) << 'c';
			}
		}
		os << print_escape(none);
		return os;
	}

	friend std::string to_string(score sc, bool colored = use_color) {
		std::string ret;
		if (sc.validated) {
			append(ret, sc.cycles);
		} else {
			if (colored) {
				ret += print_escape(red);
			}
			ret += "-";
		}
		append(ret, '/', sc.nodes, '/', sc.instructions);
		if (sc.validated and (sc.achievement or sc.cheat)) {
			ret += '/';
		}
		if (sc.validated and sc.achievement) {
			if (colored) {
				ret += print_escape(bright_blue, bold);
			}
			ret += 'a';
			if (colored) {
				ret += print_escape(none);
			}
		}
		if (sc.validated and sc.cheat) {
			if (colored) {
				ret += print_escape(yellow);
			}
			ret += 'c';
		}
		if (colored) {
			ret += print_escape(none);
		}
		return ret;
	}
};

#endif // PARSER_HPP
