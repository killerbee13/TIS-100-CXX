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
	using const_iterator_reg
	    = std::vector<std::unique_ptr<regular_node>>::const_iterator;
	using const_iterator_io
	    = std::vector<std::unique_ptr<io_node>>::const_iterator;

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
					nodes_regular.push_back(std::make_unique<T21>(x, y));
					break;
				case node::T30: {
					nodes_regular.push_back(std::make_unique<T30>(x, y, T30_size));
				} break;
				case node::Damaged:
					nodes_regular.push_back(std::make_unique<damaged>(x, y));
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

		out_nodes_offset = 0;
		for (const auto x : range(static_cast<int>(width))) {
			auto in = spec.inputs[x];
			switch (in) {
			case node::in: {
				nodes_io.push_back(std::make_unique<input_node>(x, -1));
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
			auto out = spec.outputs[x];
			switch (out) {
			case node::out: {
				nodes_io.push_back(std::make_unique<output_node>(x, height()));
			} break;
			case node::image: {
				nodes_io.push_back(std::make_unique<image_output>(x, height()));
			} break;
			case node::null:
				// pass
				break;
			default:
				throw std::invalid_argument{
				    "invalid layout spec: illegal output node"};
			}
		}
		nodes_io.shrink_to_fit();
	}

	/// Advance the field one full cycle (step and finalize)
	void step() {
		if (allT21) {
			do_step<true>();
		} else {
			do_step<false>();
		}
	}

	template <bool allT21>
	void do_step() {
		auto log = log_debug();
		log << "Field step\n";
		// evaluate code
		for (auto& p : regulars_to_sim) {
			if constexpr (allT21) {
				static_cast<T21*>(p)->step(log);
			} else {
				p->step(log);
			}
		}
		log << '\n';
		// run io nodes, this may read from regular nodes, so it must be
		// between the 2 regular methods
		for (auto& p : ios_to_sim) {
			p->execute(log);
		}
		log << '\n';
		// execute writes
		// this is a separate step to ensure a consistent propagation delay
		for (auto& p : regulars_to_sim) {
			if constexpr (allT21) {
				static_cast<T21*>(p)->finalize(log);
			} else {
				p->finalize(log);
			}
		}
	}

	bool active() const {
		bool active{};
		for (auto& out : outputs()) {
			if (out->type() == node::out) {
				auto i = static_cast<const output_node*>(out.get());
				if (not i->complete) {
					active = true;

// speed up simulator by failing early when an incorrect output is written
#if RELEASE
					if (i->wrong) {
						return false;
					}
#endif
				}
			} else if (out->type() == node::image) {
				auto i = static_cast<const image_output*>(out.get());
				if (i->wrong_pixels) {
					active = true;
				}
			}
		}
		return active;
	}

	/// Write the full state of all nodes, similar to what the game displays
	/// in its debugger but in linear order
	std::string state() const {
		std::string ret;
		for (auto& n : nodes_regular) {
			ret += n->state();
			ret += '\n';
		}
		for (auto& n : nodes_io) {
			ret += n->state();
			ret += '\n';
		}
		return ret;
	}

	// Scoring functions
	std::size_t instructions() const;
	std::size_t nodes_used() const;

	// Whether there are any input nodes attached to the field. Image test
	// pattern levels do not use inputs so only need a single test run.
	bool has_inputs() const { return out_nodes_offset != 0; }

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

	const_iterator_reg begin_regular() const noexcept {
		return nodes_regular.begin();
	}
	const_iterator_reg end_regular() const noexcept {
		return nodes_regular.end();
	}
	const_iterator_io begin_input() const noexcept { return nodes_io.begin(); }
	const_iterator_io end_input() const noexcept {
		return nodes_io.begin() + static_cast<std::ptrdiff_t>(out_nodes_offset);
	}
	const_iterator_io begin_output() const noexcept { return end_input(); }
	const_iterator_io end_output() const noexcept { return nodes_io.end(); }

	auto regulars() const noexcept
	    -> std::ranges::subrange<const_iterator_reg, const_iterator_reg> {
		return {begin_regular(), end_regular()};
	}
	auto inputs() const noexcept
	    -> std::ranges::subrange<const_iterator_io, const_iterator_io> {
		return {begin_input(), end_input()};
	}
	auto outputs() const noexcept
	    -> std::ranges::subrange<const_iterator_io, const_iterator_io> {
		return {begin_output(), end_output()};
	}

 private:
	// w*h regulars
	std::vector<std::unique_ptr<regular_node>> nodes_regular;
	// 0..w inputs, 1..w outputs
	std::vector<std::unique_ptr<io_node>> nodes_io;
	std::vector<regular_node*> regulars_to_sim;
	std::vector<io_node*> ios_to_sim;
	std::size_t width{};
	std::size_t height() const {
		if (nodes_regular.size() == 0) {
			return 0;
		}
		return nodes_regular.size() / width;
	}
	std::size_t out_nodes_offset{};
	bool allT21 = true;
};

#endif // FIELD_HPP
