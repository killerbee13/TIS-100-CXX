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
#include "logger.hpp"
#include "node.hpp"

#include <memory>
#include <string>
#include <vector>

class field {
 public:
	bool step() {
		// set up next step's IO
		// this is a separate step to ensure a consistent propagation delay
		bool r{false};
		for (auto& p : nodes) {
			if (p) {
				r |= p->step();
			}
		}
		log_debug("step() -> ", r);
		// execute
		bool f{false};
		for (auto& p : nodes) {
			if (p) {
				f |= p->finalize();
			}
		}
		log_debug("finalize() -> ", f);
		return r or f;
	}

	bool active() const {
		bool r{};
		bool all_outputs_satisfied{true};
		for (auto& p : nodes) {
			std::string log = kblib::concat("node(", p->x, ',', p->y, ") ");
			if (type(p.get()) == node::T21) {
				kblib::append(
				    log, "s = ", kblib::etoi(static_cast<const T21*>(p.get())->s));
				if (static_cast<const T21*>(p.get())->s == T21::activity::run) {
					// r = true;
				}
			} else if (type(p.get()) == node::in) {
				auto i = static_cast<const input_node*>(p.get());
				kblib::append(log, "input ", i->idx, " of ", i->inputs.size());
				if (i->idx != i->inputs.size()) {
					// r = true;
				}
			} else if (type(p.get()) == node::out) {
				auto i = static_cast<const output_node*>(p.get());
				kblib::append(log, "output ", i->outputs_received.size(), " of ",
				              i->outputs_expected.size(), "; ");
				if (i->outputs_expected != i->outputs_received) {
					r = true;
					all_outputs_satisfied = false;
					kblib::append(log, "unequal");
				} else {
					kblib::append(log, "equal");
				}
			} else {
				kblib::append(log, "inactive");
			}
			log_debug(log);
		}
		if (all_outputs_satisfied) {
			return false;
		}
		return r;
	}

	std::size_t instructions() const;
	std::size_t nodes_used() const;
	std::size_t nodes_avail() const;
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

	friend field parse(std::string_view layout, std::string_view source,
	                   std::string_view expected, int T21_size, int T30_size);
	template <bool use_nonstandard_rep>
	friend field parse_layout(std::string_view layout, int T30_size);

	auto begin() const noexcept { return nodes.begin(); }
	auto end() const noexcept { return nodes.end(); }

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
            std::string_view expected, int T21_size = def_T21_size,
            int T30_size = def_T30_size);

field parse(std::string_view layout, std::string_view source,
            const single_test& expected, int T21_size = def_T21_size,
            int T30_size = def_T30_size);

std::vector<instr> assemble(std::string_view source, int node,
                            int T21_size = def_T21_size);

struct score {
	int cycles{};
	int nodes{};
	int instructions{};
	bool validated{};
};

#endif // PARSER_HPP
