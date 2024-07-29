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

	/// Advance the field one full cycle (step and finalize), returning true if
	/// any work done
	bool step() {
		// evaluate code
		bool r{false};
		for (auto& p : nodes) {
			if (p) {
				r |= p->step();
			}
		}
		log_debug("step() -> ", r, '\n');
		// execute writes
		// this is a separate step to ensure a consistent propagation delay
		bool f{false};
		for (auto& p : nodes) {
			if (p) {
				f |= p->finalize();
			}
		}
		log_debug("finalize() -> ", f, '\n');
		return r or f;
	}

	bool active() const {
		bool r{};
		bool all_outputs_satisfied{true};
		bool all_outputs_correct{true};
		for (auto& p : nodes) {
			if (type(p.get()) == node::T21) {
				//	if (static_cast<const T21*>(p.get())->s == activity::run) {
				//		r = true;
				//	}
			} else if (type(p.get()) == node::in) {
				// if (i->idx != i->inputs.size()) {
				//	 r = true;
				// }
			} else if (type(p.get()) == node::out) {
				auto i = static_cast<const output_node*>(p.get());
				if (i->outputs_received.size() < i->outputs_expected.size()) {
					r = true;
					all_outputs_satisfied = false;

					if (i->outputs_received.size() > i->outputs_expected.size()) {
						all_outputs_correct = false;
					}
// speed up simulator by failing early when an incorrect output is written
#if RELEASE
					else if (auto k = i->outputs_received.size() - 1;
					         not i->outputs_received.empty()
					         and i->outputs_received[k] != i->outputs_expected[k]) {
						all_outputs_correct = false;
					}
#endif
				}
			} else if (type(p.get()) == node::image) {
				auto i = static_cast<const image_output*>(p.get());
				if (i->image_expected != i->image_received) {
					r = true;
					all_outputs_satisfied = false;
				}
			}
		}
		if (all_outputs_satisfied or not all_outputs_correct) {
			return false;
		}
		return r;
	}

	/// Write the full state of all nodes, similar to what the game displays in
	/// its debugger but in linear order
	std::string state() {
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
	std::size_t nodes_total() const { return io_node_offset; }
	// Whether there are any input nodes attached to the field. Image test
	// pattern levels do not use inputs so only need a single test run.
	bool has_inputs() const {
		for (auto it = begin() + static_cast<std::ptrdiff_t>(nodes_total());
		     it != end(); ++it) {
			if (auto p = it->get(); type(p) == node::in) {
				log_debug("Field has input at location ", p->x);
				return true;
			}
		}
		log_debug("Field has no inputs");
		return false;
	}

	/// Serialize layout as read by parse_layout
	std::string layout() const;
	/// Serialize layout as a C++ builtin_layout_spec initializer
	std::string machine_layout() const;

	// returns the node at the (x,y) coordinates
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
		for (auto& p : nodes) {
			if (type(p.get()) == node::T21 and i-- == 0) {
				return static_cast<T21*>(p.get());
			}
		}
		return nullptr;
	}
	const T21* node_by_index(std::size_t i) const {
		for (auto& p : nodes) {
			if (type(p.get()) == node::T21 and i-- == 0) {
				return static_cast<const T21*>(p.get());
			}
		}
		return nullptr;
	}

	template <bool use_nonstandard_rep>
	friend field parse_layout(std::string_view layout, std::size_t T30_size);

	const_iterator begin() const noexcept { return nodes.begin(); }
	// Partition between regular and IO nodes
	const_iterator end_regular() const noexcept {
		return nodes.begin() + static_cast<std::ptrdiff_t>(io_node_offset);
	}
	const_iterator end() const noexcept { return nodes.end(); }

 private:
	std::vector<std::unique_ptr<node>> nodes;
	std::size_t width{};
	std::size_t height() const {
		if (io_node_offset == 0) {
			return 0;
		}
		return io_node_offset / width;
	}
	// must be a multiple of width
	std::size_t io_node_offset{};
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
