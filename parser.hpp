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

#include "node.hpp"

#include <memory>
#include <string>
#include <vector>

class field {
 public:
	void step() {
		// set up next step's IO
		// this is a separate step to ensure a consistent propagation delay
		for (auto& p : nodes) {
			if (p) {
				p->read();
			}
		}
		// execute
		for (auto& p : nodes) {
			if (p) {
				p->step();
			}
		}
	}

	std::size_t instructions() const;
	std::size_t nodes_used() const;
	std::size_t nodes_total() const { return io_node_offset; }

	std::string layout() const;

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
	node* node_by_index(std::size_t i) {
		for (auto& p : nodes) {
			if (p and p->type() == node::T21 and --i == 0) {
				return p.get();
			}
		}
		return nullptr;
	}
	const node* node_by_index(std::size_t i) const {
		for (auto& p : nodes) {
			if (p and p->type() == node::T21 and --i == 0) {
				return p.get();
			}
		}
		return nullptr;
	}

	friend field parse(std::string_view layout, std::string_view source,
	                   std::string_view expected, int T21_size, int T30_size);
	friend field parse_layout(std::string_view layout);

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

field parse(std::string_view layout, std::string_view source,
            std::string_view expected, int T21_size = 15, int T30_size = 15);

#endif // PARSER_HPP
