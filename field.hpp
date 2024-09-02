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
#ifndef FIELD_HPP
#define FIELD_HPP

#include "T21.hpp"
#include "T30.hpp"
#include "io.hpp"
#include <memory>

class field {
	using data_t = std::vector<std::unique_ptr<node>>;
	using iterator = data_t::iterator;
	using const_iterator = data_t::const_iterator;

 public:
	field() = default;
	template <typename Spec>
	field(const Spec& spec, std::size_t T30_size = def_T30_size) {
		if (spec.nodes.empty()) {
			return;
		}
		width = spec.nodes[0].size();
		if (spec.io[0].size() != width or spec.io[1].size() != width) {
			throw std::invalid_argument{
			    "Layout IO specs must match field dimensions"};
		}
		nodes.reserve([&] {
			std::size_t r = 12;
			for (auto t : spec.io) {
				for (auto i : t) {
					if (i != node::null) {
						++r;
					}
				}
			}
			return r;
		}());

		in_nodes_offset = spec.nodes.size() * width;
		out_nodes_offset = in_nodes_offset;
		for (auto y : range(spec.nodes.size())) {
			if (spec.nodes[y].size() != width) {
				throw std::invalid_argument{"Layout specs must be rectangular"};
			}
			for (auto x : range(width)) {
				switch (spec.nodes[y][x]) {
				case node::T21:
					nodes.push_back(std::make_unique<T21>(x, y));
					break;
				case node::T30: {
					nodes.push_back(std::make_unique<T30>(x, y, T30_size));
				} break;
				case node::Damaged:
					nodes.push_back(std::make_unique<damaged>(x, y));
					break;
				case node::in:
				case node::out:
				case node::image:
					throw std::invalid_argument{
					    "invalid layout spec: IO node as regular node"};
				case node::null:
					throw std::invalid_argument{
					    "invalid layout spec: null node as regular node"};
				}
			}
		}
		for (const auto x : range(static_cast<int>(width))) {
			auto in = spec.io[0][x];
			switch (in) {
			case node::in: {
				nodes.push_back(std::make_unique<input_node>(x, -1));
				out_nodes_offset++;
			} break;
			case node::null:
				// pass
				break;
			default:
				throw std::invalid_argument{
				    "invalid layout spec: illegal input node"};
			}
		}

		for (const auto x : range(static_cast<int>(width))) {
			auto out = spec.io[1][x];
			switch (out) {
			case node::out: {
				nodes.push_back(std::make_unique<output_node>(x, height()));
			} break;
			case node::image: {
				nodes.emplace_back(std::make_unique<image_output>(x, height()));
			} break;
			case node::null:
				// pass
				break;
			default:
				throw std::invalid_argument{
				    "invalid layout spec: illegal output node"};
			}
		}
	}
	// field(builtin_layout_spec spec, std::size_t T30_size = def_T30_size);

	/// Advance the field one full cycle (step and finalize)
	void step() {
		auto log = log_debug();
		log << "Field step\n";
		// evaluate code
		for (auto& p : nodes_to_sim) {
			p->step(log);
		}
		log << '\n';
		// execute writes
		// this is a separate step to ensure a consistent propagation delay
		for (auto& p : nodes_to_sim) {
			p->finalize(log);
		}
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
				if (i->wrong_pixels) {
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
			ret += n->state();
			ret += '\n';
		}
		return ret;
	}

	// Scoring functions
	std::size_t instructions() const;
	std::size_t nodes_used() const;

	// Number of regular (grid) nodes
	std::size_t nodes_total() const { return in_nodes_offset; }
	// Whether there are any input nodes attached to the field. Image test
	// pattern levels do not use inputs so only need a single test run.
	bool has_inputs() const { return in_nodes_offset != out_nodes_offset; }

	/// Serialize human-readable layout
	std::string layout() const;
	/// Serialize layout as a C++ builtin_layout_spec initializer
	std::string machine_layout() const;

	/// must be called after code loading
	void finalize_nodes();
	/// returns field with all nodes cloned and resetted
	field clone() const;

	// returns the node at the (x,y) coordinates, Nullable
	node* reg_node_by_location(std::size_t x, std::size_t y) {
		if (x >= width or y >= height()) {
			return nullptr;
		}
		auto i = y * width + x;
		assert(i < in_nodes_offset);
		return nodes[i].get();
	}
	// Nullable
	const node* reg_node_by_location(std::size_t x, std::size_t y) const {
		if (x >= width or y >= height()) {
			return nullptr;
		}
		auto i = y * width + x;
		assert(i < in_nodes_offset);
		return nodes[i].get();
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
	// <= ^ regulars, ^ inputs, ^ outputs
	std::vector<node*> nodes_to_sim;
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

/// null and Damaged are negative. Any positive value is a valid node
// (0 is unallocated)
inline bool valid(const node* n) { return n and etoi(n->type()) > 0; }
/// nodes that need to be simulated, implies valid
inline bool useful(const node* n) {
	if (not n) {
		return false;
	}
	switch (n->type()) {
	case node::T21:
		return not static_cast<const T21*>(n)->code.empty();
	case node::T30:
	case node::in:
	case node::out:
	case node::image:
		return true;
	default:
		return false;
	}
}

#endif // FIELD_HPP
