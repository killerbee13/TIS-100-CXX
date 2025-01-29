/* *****************************************************************************
 * TIS-100-CXX
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
#ifndef FIELD_HPP
#define FIELD_HPP

#include "T21.hpp"
#include "T30.hpp"
#include "io.hpp"
#include "node.hpp"

#include <memory>

/// nodes that are candidates to be simulated
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

class field {
 public:
	field() = default;
	template <typename Spec>
	   requires requires(Spec s) {
		   { s.nodes[0] };
		   { s.inputs[0] };
		   { s.outputs[0] };
	   }
	field(const Spec& spec, std::size_t T30_size = def_T30_size) {
		if (spec.nodes.empty()) {
			return;
		}
		width = spec.nodes[0].size();
		if (spec.inputs.size() != width or spec.outputs.size() != width) {
			throw std::invalid_argument{
			    "Layout IO specs must match field dimensions"};
		}
		nodes_regular.reserve(width * spec.nodes.size());

		for (auto y : range(spec.nodes.size())) {
			if (spec.nodes[y].size() != width) {
				throw std::invalid_argument{"Layout specs must be rectangular"};
			}
			for (auto x : range(width)) {
				switch (spec.nodes[y][x]) {
				case node::T21:
					nodes_regular.push_back(std::make_unique<T21>(
					    static_cast<int>(x), static_cast<int>(y)));
					break;
				case node::T30: {
					nodes_regular.push_back(std::make_unique<T30>(
					    static_cast<int>(x), static_cast<int>(y), T30_size));
				} break;
				case node::Damaged:
					nodes_regular.push_back(std::make_unique<damaged>(
					    static_cast<int>(x), static_cast<int>(y)));
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
			auto in = spec.inputs[x];
			switch (in) {
			case node::in: {
				nodes_input.push_back(std::make_unique<input_node>(x, -1));
			} break;
			case node::null:
				// pass
				break;
			default:
				throw std::invalid_argument{
				    "invalid layout spec: illegal input node"};
			}
		}
		nodes_input.shrink_to_fit();

		for (const auto x : range(static_cast<int>(width))) {
			auto out = spec.outputs[x];
			switch (out) {
			case node::out: {
				nodes_numeric.push_back(
				    std::make_unique<num_output>(x, static_cast<int>(height())));
			} break;
			case node::image: {
				nodes_image.push_back(
				    std::make_unique<image_output>(x, static_cast<int>(height())));
			} break;
			case node::null:
				// pass
				break;
			default:
				throw std::invalid_argument{
				    "invalid layout spec: illegal output node"};
			}
		}
		nodes_numeric.shrink_to_fit();
		nodes_image.shrink_to_fit();
	}

	/// Advance the field one full cycle (step and finalize)
	bool step() {
		if (allT21) {
			return do_step<true>();
		} else {
			return do_step<false>();
		}
	}

	template <bool allT21>
	bool do_step() {
		auto debug = log_debug();
		debug << "Field step\n";
		// evaluate code
		for (auto& p : regulars_to_sim) {
			if constexpr (allT21) {
				static_cast<T21*>(p)->step(debug);
			} else {
				p->step(debug);
			}
		}
		debug << '\n';

		// run io nodes, this may read from regular nodes, so it must be
		// between the 2 regular methods
		for (auto& p : inputs_to_sim) {
			p->execute(debug);
		}
		bool active = false;
		for (auto& p : numerics_to_sim) {
			active |= p->execute(debug);
		}
		for (auto& p : images_to_sim) {
			active |= p->execute(debug);
		}
		debug << '\n';

		// execute writes
		// this is a separate step to ensure a consistent propagation delay
		for (auto& p : regulars_to_sim) {
			if constexpr (allT21) {
				static_cast<T21*>(p)->finalize(debug);
			} else {
				p->finalize(debug);
			}
		}
		return active;
	}

	/// Write the full state of all nodes, similar to what the game displays
	/// in its debugger but in linear order
	std::string state() const {
		std::string ret;
		for (auto& n : nodes_input) {
			ret += n->state();
			ret += '\n';
		}
		for (auto& n : nodes_regular) {
			ret += n->state();
			ret += '\n';
		}
		for (auto& n : nodes_numeric) {
			ret += n->state();
			ret += '\n';
		}
		for (auto& n : nodes_image) {
			ret += n->state();
			ret += '\n';
		}
		return ret;
	}

	// Scoring functions
	std::size_t instructions() const;
	std::size_t nodes_used() const;

	/// Serialize human-readable layout
	std::string layout() const;

	/// must be called after code loading
	void finalize_nodes();
	/// returns field with all nodes cloned and resetted
	field clone() const;

	/// returns the node at the (x,y) coordinates, or nullptr if such a node
	/// doesn't exist or is not useful
	regular_node* useful_node_at(std::size_t x, std::size_t y) {
		if (x >= width or y >= height()) {
			return nullptr;
		}
		auto i = y * width + x;
		auto* p = nodes_regular[i].get();
		return useful(p) ? p : nullptr;
	}
	// returns the ith programmable (T21) node
	T21* node_by_index(std::size_t i) {
		for (auto& n : nodes_regular) {
			auto p = n.get();
			if (p->type() == node::T21 and i-- == 0) {
				return static_cast<T21*>(p);
			}
		}
		return nullptr;
	}

	const auto& inputs() const noexcept { return nodes_input; }
	const auto& regulars() const noexcept { return nodes_regular; }
	const auto& numerics() const noexcept { return nodes_numeric; }
	const auto& images() const noexcept { return nodes_image; }
	void for_each_output(std::invocable<output_node*> auto f) {
		for (auto& n : nodes_numeric) {
			f(n.get());
		}
		for (auto& n : nodes_image) {
			f(n.get());
		}
	}

 private:
	std::vector<std::unique_ptr<input_node>> nodes_input;
	std::vector<std::unique_ptr<regular_node>> nodes_regular;
	std::vector<std::unique_ptr<num_output>> nodes_numeric;
	std::vector<std::unique_ptr<image_output>> nodes_image;

	std::vector<input_node*> inputs_to_sim;
	std::vector<regular_node*> regulars_to_sim;
	std::vector<num_output*> numerics_to_sim;
	std::vector<image_output*> images_to_sim;

	std::size_t width{};
	std::size_t height() const {
		if (nodes_regular.size() == 0) {
			return 0;
		}
		return nodes_regular.size() / width;
	}
	bool allT21 = true;

	bool search_for_output(const regular_node*);
};

#endif // FIELD_HPP
