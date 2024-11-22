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

#include "field.hpp"

#include <bitset>
#include <queue>
#include <unordered_set>

using dir_mask = std::bitset<DIMENSIONS * 2>;

constexpr dir_mask port_mask(port p) {
	if (p >= port::dir_first and p <= port::dir_last) {
		return dir_mask{}.set(p);
	} else if (p == port::any) {
		return dir_mask{}.set();
	} else {
		return {};
	}
}

constexpr dir_mask in_links(const regular_node* n) {
	if (auto p = dynamic_cast<const T21*>(n)) {
		auto mask = dir_mask{};
		bool reads_from_last{};
		bool writes_to_any{};
		for (auto i : p->code) {
			switch (i.op_) {
			case instr::mov:
				if (i.dst == any) {
					writes_to_any = true;
				}
				[[fallthrough]];
			case instr::add:
			case instr::sub:
			case instr::jro:
				mask |= port_mask(i.src);
				if (i.src == last) {
					reads_from_last = true;
				}
				break;
			default:
				break;
			}
		}
		if (reads_from_last and writes_to_any) {
			mask.set();
		}
		return mask;
	} else if (dynamic_cast<const T30*>(n)) {
		return dir_mask{}.set();
	} else {
		return {};
	}
}
constexpr dir_mask out_links(const regular_node* n) {
	if (auto p = dynamic_cast<const T21*>(n)) {
		auto mask = dir_mask{};
		bool reads_from_any{};
		bool writes_to_last{};
		for (auto i : p->code) {
			switch (i.op_) {
			case instr::mov:
				mask |= port_mask(i.dst);
				if (i.dst == last) {
					writes_to_last = true;
				}
				[[fallthrough]];
			case instr::add:
			case instr::sub:
			case instr::jro:
				if (i.src == any) {
					reads_from_any = true;
				}
				break;
			default:
				break;
			}
		}
		if (reads_from_any and writes_to_last) {
			mask.set();
		}
		return mask;
	} else if (dynamic_cast<const T30*>(n)) {
		return dir_mask{}.set();
	} else {
		return {};
	}
}

static constexpr std::array<std::pair<int, int>, 2 * DIMENSIONS> delta_lookup{
    {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};

// search for an output node connected (by imask U omask) to this one
bool field::search_for_output(const regular_node* p) {
	struct entry {
		const regular_node* n;
		// use height as heuristic, because outputs are always at bottom
		bool operator<(const entry& rhs) const { return n->y < rhs.n->y; }
	};
	// std::priority_queue defaults to a max-heap
	std::priority_queue<entry> queue;
	std::unordered_set<const regular_node*> searched;
	auto pop = [&] {
		auto tmp = queue.top();
		queue.pop();
		return tmp;
	};
	{
		queue.push({p});
		searched.insert(p);
	}

	while (not queue.empty()) {
		auto n = pop().n;
		log_debug("Searching node (", n->x, ", ", n->y, ")");
		if (n->type() == node::T30) {
			if (std::ranges::none_of(n->neighbors, std::identity{})) {
				continue;
			}
		}
		for (auto d = port::dir_first; d <= port::dir_last; ++d) {
			auto neighbor_x = n->x + delta_lookup[d].first;
			auto neighbor_y = n->y + delta_lookup[d].second;
			auto neighbor = useful_node_at(neighbor_x, neighbor_y);
			if (neighbor) {
				if ((n->neighbors[d] or neighbor->neighbors[invert(d)])
				    and searched.insert(neighbor).second) {
					if (neighbor->type() == node::T21
					    and static_cast<const T21*>(neighbor)->has_instr(
					        instr::hcf)) {
						log_debug("Neighbor has hcf");
						return true;
					}
					queue.push({neighbor});
				}
			} else if (neighbor_y == static_cast<int>(height())) {
				for (auto& o : nodes_numeric) {
					if (o->x == neighbor_x) {
						log_debug("Neighbor is numeric output");
						return true;
					}
				}
				for (auto& o : nodes_image) {
					if (o->x == neighbor_x) {
						log_debug("Neighbor is image output");
						return true;
					}
				}
			}
		}
	}
	return false;
}

void field::finalize_nodes() {
	// set links
	for (auto& n : nodes_regular) {
		auto p = n.get();
		if (not useful(p)) {
			log_debug("node (", p->x, ", ", p->y, ") : Not useful");
			continue;
		}

		auto imask = in_links(p);
		log_debug("Node at (", p->x, ", ", p->y, ") imask: ", imask.to_string());
		for (auto d = port::dir_first; d <= port::dir_last; ++d) {
			auto neighbor_x = p->x + delta_lookup[d].first;
			auto neighbor_y = p->y + delta_lookup[d].second;
			auto neighbor = useful_node_at(neighbor_x, neighbor_y);
			if (neighbor) {
				auto omask = out_links(neighbor);
				log_debug_r([&] {
					return concat("\tneighbor[", etoi(d), "] (", neighbor->x, ", ",
					              neighbor->y, ") omask:", omask.to_string(), "; ",
					              bool(imask[d]), bool(omask[invert(d)]), ' ',
					              imask[d] and omask[invert(d)] ? "linked"
					                                            : "unlinked");
				});
				// a link only needs to be from dest to source (because it's
				// only used to call emit), and only if that source can
				// actually write to this dest
				if (imask[d] and omask[invert(d)]) {
					p->neighbors[d] = neighbor;
				}
			}
		}

		log_debug_r([&] {
			std::string ret
			    = concat("node at (", p->x, ',', p->y, ") has neighbors: ");
			for (auto neighbor : p->neighbors) {
				if (neighbor) {
					append(ret, " (", neighbor->x, ',', neighbor->y,
					       "): ", etoi(neighbor->type()), ", ");
				}
			}
			return ret;
		});
	}
	for (auto& i : nodes_input) {
		auto n = useful_node_at(i->x, 0);
		if (n) {
			n->neighbors[up] = i.get();
			log_debug("input node at (", i->x, ',', i->y, ") has neighbor: (",
			          n->x, ',', n->y, "): ", etoi(n->type()));
		}
	}
	for_each_output([this](auto* o) {
		auto n = useful_node_at(o->x, height() - 1);
		o->linked = n;
		if (n) {
			log_debug("output node at (", o->x, ", ", o->y, ") has neighbor: (",
			          n->x, ", ", n->y, "): ", etoi(n->type()));
		}
	});

	// register for simulation
	for (auto& p : nodes_regular) {
		if (useful(p.get())) {
			if (search_for_output(p.get())) {
				log_debug("node at (", p->x, ", ", p->y, ") marked useful");
				regulars_to_sim.push_back(p.get());
				allT21 &= p->type() == node::T21;
			} else {
				log_debug("node at (", p->x, ", ", p->y,
				          ") dropped as not connected");
			}
		} else {
			log_debug("node at (", p->x, ", ", p->y, ") dropped as not useful");
		}
	}
	if (allT21) {
		log_debug("All used regular nodes are T21, faster simulation enabled");
	}
	for (auto& i : nodes_input) {
		auto n = useful_node_at(i->x, 0);
		if (n and search_for_output(n)) {
			inputs_to_sim.push_back(i.get());
		} else {
			log_debug("Input node at (", i->x, ", ", i->y, ") dropped");
		}
	}
	for (auto& o : nodes_numeric) {
		if (o->linked) {
			numerics_to_sim.push_back(o.get());
		} else {
			// Output nodes not being connected will make the level unsolvable
			// (unless the output is always empty/blank)
			log_info("Output node at (", o->x, ", ", o->y, ") dropped");
		}
	}
	for (auto& o : nodes_image) {
		if (o->linked) {
			images_to_sim.push_back(o.get());
		} else {
			// Output nodes not being connected will make the level unsolvable
			// (unless the output is always empty/blank)
			log_info("Output node at (", o->x, ", ", o->y, ") dropped");
		}
	}
}

std::size_t field::instructions() const {
	std::size_t ret{};
	for (auto& i : nodes_regular) {
		auto p = i.get();
		if (p->type() == node::T21) {
			ret += static_cast<T21*>(p)->code.size();
		}
	}
	return ret;
}

std::size_t field::nodes_used() const {
	std::size_t ret{};
	for (auto& i : nodes_regular) {
		auto p = i.get();
		if (p->type() == node::T21) {
			ret += not static_cast<T21*>(p)->code.empty();
		}
	}
	return ret;
}

std::string field::layout() const {
	std::string ret;
	for (auto& in : nodes_input) {
		append(ret, 'I', in->x);
		if (not in->inputs.empty()) {
			append(ret, " [");
			for (auto v : in->inputs) {
				append(ret, v, ", ");
			}
			append(ret, "]");
		}
		append(ret, ' ');
	}

	for (auto [p, i] : kblib::enumerate(nodes_regular)) {
		if (i % width == 0) {
			ret += '\n';
		}
		if (p->type() == node::Damaged) {
			ret += 'D';
		} else if (p->type() == node::T21) {
			ret += 'C';
		} else if (p->type() == node::T30) {
			ret += 'S';
		}
	}
	ret += '\n';

	for (auto& on : nodes_numeric) {
		append(ret, 'O', on->x);
		if (not on->outputs_expected.empty()) {
			append(ret, " [");
			for (auto v : on->outputs_expected) {
				append(ret, v, ", ");
			}
			append(ret, "]");
		}
		append(ret, ' ');
	}
	for (auto& im : nodes_image) {
		append(ret, 'V', im->x, " ", im->width, ',', im->height);
		if (not im->image_expected.blank()) {
			append(ret, " [", im->image_expected.write_text(false), "]");
		}
		append(ret, ' ');
	}

	return ret;
}

field field::clone() const {
	field ret;

	for (auto& n : nodes_input) {
		ret.nodes_input.push_back(n->clone());
	}
	for (auto& n : nodes_regular) {
		ret.nodes_regular.push_back(n->clone());
	}
	for (auto& n : nodes_numeric) {
		ret.nodes_numeric.push_back(n->clone());
	}
	for (auto& n : nodes_image) {
		ret.nodes_image.push_back(n->clone());
	}

	ret.width = width;

	ret.finalize_nodes();

	return ret;
}
