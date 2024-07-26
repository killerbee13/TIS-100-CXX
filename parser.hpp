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
	using data_t = std::vector<std::unique_ptr<node>>;
	using iterator = data_t::iterator;
	using const_iterator = data_t::const_iterator;

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
		log_debug("step() -> ", r, '\n');
		// execute
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
		auto log = log_debug();
		for (auto& p : nodes) {
			log.log_r(
			    [&] { return kblib::concat("node(", p->x, ',', p->y, ") "); });
			if (type(p.get()) == node::T21) {
				log.log("s = ", state_name(static_cast<const T21*>(p.get())->s));
				//	if (static_cast<const T21*>(p.get())->s == activity::run) {
				//		r = true;
				//	}
			} else if (type(p.get()) == node::in) {
				auto i = static_cast<const input_node*>(p.get());
				log.log("input ", i->idx, " of ", i->inputs.size());
				// if (i->idx != i->inputs.size()) {
				//	 r = true;
				// }
			} else if (type(p.get()) == node::out) {
				auto i = static_cast<const output_node*>(p.get());
				log.log("output ", i->outputs_received.size(), " of ",
				        i->outputs_expected.size(), "; ");
				if (i->outputs_received.size() < i->outputs_expected.size()) {
					r = true;
					all_outputs_satisfied = false;
					log << "unfilled";

					if (i->outputs_received.size() > i->outputs_expected.size()) {
						all_outputs_correct = false;
					}
// speed up simulator by failing early when an incorrect output is written
#if RELEASE
					else if (auto k = i->outputs_received.size() - 1;
					         not i->outputs_received.empty()
					         and i->outputs_received[k] != i->outputs_expected[k]) {
						all_outputs_correct = false;
						log << " incorrect value written for output O" << i->x;
					}
#endif

				} else {
					log << "filled";
				}
			} else if (type(p.get()) == node::image) {
				auto i = static_cast<const image_output*>(p.get());
				if (i->image_expected != i->image_received) {
					r = true;
					all_outputs_satisfied = false;
					log << "unequal";
				} else {
					log << "equal";
				}
			} else {
				log << "inactive";
			}
			log << '\n';
		}
		if (all_outputs_satisfied or not all_outputs_correct) {
			return false;
		}
		return r;
	}

	std::string state() {
		std::string ret;
		for (auto& n : nodes) {
			ret += n->print();
			ret += '\n';
		}
		return ret;
	}

	std::size_t instructions() const;
	std::size_t nodes_used() const;
	std::size_t nodes_avail() const;
	std::size_t nodes_total() const { return io_node_offset; }
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
	                   std::string_view expected, std::size_t T21_size,
	                   std::size_t T30_size);
	template <bool use_nonstandard_rep>
	friend field parse_layout(std::string_view layout, std::size_t T30_size);

	const_iterator begin() const noexcept { return nodes.begin(); }
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

field parse(std::string_view layout, std::string_view source,
            std::string_view expected, std::size_t T21_size = def_T21_size,
            std::size_t T30_size = def_T30_size);

field parse(std::string_view layout, std::string_view source,
            const single_test& expected, std::size_t T21_size = def_T21_size,
            std::size_t T30_size = def_T30_size);
field parse_layout_guess(std::string_view layout, std::size_t T30_size);

std::vector<instr> assemble(std::string_view source, int node,
                            std::size_t T21_size = def_T21_size);
void parse_code(field& f, std::string_view source, std::size_t T21_size);
void set_expected(field& f, const single_test& expected);

struct score {
	std::size_t cycles{};
	std::size_t nodes{};
	std::size_t instructions{};
	bool validated{};
	bool achievement{};
	bool cheat{};

	friend std::ostream& operator<<(std::ostream& os, score sc) {
		if (sc.validated) {
			os << sc.cycles << '/';
		} else {
			os << print_color(red) << "-/";
		}
		os << sc.nodes << '/' << sc.instructions;
		if (sc.validated and (sc.achievement or sc.cheat)) {
			os << '/';
		}
		if (sc.validated and sc.achievement) {
			os << print_color(bright_blue) << 'a';
		}
		if (sc.validated and sc.cheat) {
			os << print_color(yellow) << 'c';
		}
		os << print_color(reset);
		return os;
	}

	friend std::string to_string(score sc, bool colored = use_color) {
		std::string ret;
		if (sc.validated) {
			kblib::append(ret, sc.cycles);
		} else {
			if (colored) {
				ret += print_color(red);
			}
			ret += "-";
		}
		kblib::append(ret, '/', sc.nodes, '/', sc.instructions);
		if (sc.validated and (sc.achievement or sc.cheat)) {
			ret += '/';
		}
		if (sc.validated and sc.achievement) {
			if (colored) {
				ret += print_color(bright_blue);
			}
			ret += 'a';
		}
		if (sc.validated and sc.cheat) {
			if (colored) {
				ret += print_color(yellow);
			}
			ret += 'c';
		}
		if (colored) {
			ret += print_color(reset);
		}
		return ret;
	}
};

#endif // PARSER_HPP
