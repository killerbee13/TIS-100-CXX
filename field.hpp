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
#include "layoutspecs.hpp"
#include <memory>

class field {
	using data_t = std::vector<std::unique_ptr<node>>;
	using iterator = data_t::iterator;
	using const_iterator = data_t::const_iterator;

 public:
	field() = default;
	field(builtin_layout_spec spec, std::size_t T30_size = def_T30_size);

	/// Advance the field one full cycle (step and finalize)
	void step() {
		auto log = log_debug();
		log << "Field step\n";
		// evaluate code
		for (auto& p : nodes_active) {
			p->step(log);
		}
		log << '\n';
		// execute writes
		// this is a separate step to ensure a consistent propagation delay
		for (auto& p : nodes_active) {
			p->finalize(log);
		}
	}

	bool active() const {
		bool active{};
		for (auto& n : nodes_output) {
			if (not n.complete) {
				active = true;

// speed up simulator by failing early when an incorrect output is written
#if RELEASE
				if (n.wrong) {
					return false;
				}
#endif
			}
		}
		for (auto& n : nodes_image) {
			if (n.image_expected != n.image_received) {
				active = true;
			}
		}
		return active;
	}

	/// Write the full state of all nodes, similar to what the game displays in
	/// its debugger but in linear order
	std::string state() const {
		std::string ret;
		for (auto& n : all_nodes) {
			ret += n->state();
			ret += '\n';
		}
		return ret;
	}

	// Scoring functions
	std::size_t instructions() const;
	std::size_t nodes_used() const;

	// Number of regular (grid) nodes
	std::size_t nodes_total() const { return width * height; }
	// Whether there are any input nodes attached to the field. Image test
	// pattern levels do not use inputs so only need a single test run.
	bool has_inputs() const { return not nodes_input.empty(); }

	/// Serialize layout as read by parse_layout
	std::string layout() const;
	/// Serialize layout as a C++ builtin_layout_spec initializer
	std::string machine_layout() const;

	field clone() const;

	// returns the node at the (x,y) coordinates, Nullable
	node* node_by_location(std::size_t x, std::size_t y) {
		if (x > width) {
			return nullptr;
		}
		auto i = y * width + x;
		if (i < all_nodes.size()) {
			return all_nodes[i];
		} else {
			return nullptr;
		}
	}
	// Nullable
	const node* node_by_location(std::size_t x, std::size_t y) const {
		if (x > width or y > height) {
			return nullptr;
		}
		auto i = y * width + x;
		if (i < all_nodes.size()) {
			return all_nodes[i];
		} else {
			return nullptr;
		}
	}

	void set_neighbors();


	// node arenas
	std::vector<T21> nodes_T21;
	std::vector<T30> nodes_T30;
	std::vector<damaged> nodes_damaged;
	std::vector<input_node> nodes_input;
	std::vector<output_node> nodes_output;
	std::vector<image_output> nodes_image;
	// w*h regulars, 0..w inputs, 1..w outputs
	std::vector<node*> all_nodes;
	// excludes damaged and empty T21s
	std::vector<node*> nodes_active;
 private:
	std::size_t width{};
	std::size_t height{};
};

#endif // FIELD_HPP
