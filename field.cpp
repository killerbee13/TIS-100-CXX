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
#include "layoutspecs.hpp"

void field::finalize_nodes() {
	for (auto it = begin_regular(); it != end_regular(); ++it) {
		if (auto p = it->get(); useful(p)) {
			int x = p->x;
			int y = p->y;
			// safe because useful_node_at returns nullptr on OOB
			p->neighbors[left] = useful_node_at(x - 1, y);
			p->neighbors[right] = useful_node_at(x + 1, y);
			p->neighbors[up] = useful_node_at(x, y - 1);
			p->neighbors[down] = useful_node_at(x, y + 1);
		}
	}

	for (auto it = begin_io(); it != end_input(); ++it) {
		auto p = it->get();
		auto n = useful_node_at(p->x, 0);
		p->linked = n;
		if (n) {
			n->neighbors[up] = p;
		}
	}

	for (auto it = begin_output(); it != end_output(); ++it) {
		auto p = it->get();
		auto n = useful_node_at(p->x, height() - 1);
		p->linked = n;
		if (n) {
			n->neighbors[down] = p;
		}
	}

	for (auto& p : nodes_regular) {
		// useful and connected to useful nodes
		// nodes with HCF need to be simulated even if they are isolated
		if (useful(p.get())) {
			bool connected = std::ranges::any_of(p->neighbors,
			                                     [](auto n) { return bool(n); });
			if (connected
			    or (p->type() == node::T21
			        and static_cast<const T21*>(p.get())->has_instr(instr::hcf))) {
				log_debug("node at (", p->x, ',', p->y, ") marked useful");
				regulars_to_sim.push_back(p.get());
			}
		}
	}
	for (auto& p : nodes_io) {
		if (useful(p.get()) and p->linked) {
			log_debug("io node at (", p->x, ',', p->y, ") marked useful");
			ios_to_sim.push_back(p.get());
		}
	}
}

std::size_t field::instructions() const {
	std::size_t ret{};
	for (auto it = begin_regular(); it != end_regular(); ++it) {
		auto p = it->get();
		if (p->type() == node::T21) {
			ret += static_cast<T21*>(p)->code.size();
		}
	}
	return ret;
}

std::size_t field::nodes_used() const {
	std::size_t ret{};
	for (auto it = begin_regular(); it != end_regular(); ++it) {
		auto p = it->get();
		if (p->type() == node::T21) {
			ret += not static_cast<T21*>(p)->code.empty();
		}
	}
	return ret;
}

std::string field::layout() const {
	auto h = height();
	std::string ret = concat(width, ' ', h, '\n');
	for (auto it = begin_regular(); it != end_regular(); ++it) {
		auto p = it->get();
		if (p->type() == node::Damaged) {
			ret += 'D';
		} else if (p->type() == node::T21) {
			ret += 'C';
		} else if (p->type() == node::T30) {
			ret += 'S';
		}
	}
	ret += '\n';

	for (auto it = begin_io(); it != end_output(); ++it) {
		auto p = it->get();

		if (p->type() == node::in) {
			auto in = static_cast<input_node*>(p);
			append(ret, 'I', p->x);
			if (not in->inputs.empty()) {
				append(ret, " [");
				for (auto v : in->inputs) {
					append(ret, v, ", ");
				}
				append(ret, "]");
			}
			append(ret, ' ');
		} else if (p->type() == node::out) {
			auto on = static_cast<output_node*>(p);
			append(ret, 'O', p->x);
			if (not on->outputs_expected.empty()) {
				append(ret, " [");
				for (auto v : on->outputs_expected) {
					append(ret, v, ", ");
				}
				append(ret, "]");
			}
			append(ret, ' ');
		} else if (p->type() == node::image) {
			auto im = static_cast<image_output*>(p);
			append(ret, 'V', p->x, " ", im->width, ',', im->height);
			if (not im->image_expected.blank()) {
				append(ret, " [", im->image_expected.write_text(false), "]");
			}
			append(ret, ' ');
		}
	}

	return ret;
}

field field::clone() const {
	field ret;

	for (auto& n : nodes_regular) {
		ret.nodes_regular.push_back(n->clone());
	}
	for (auto& n : nodes_io) {
		ret.nodes_io.push_back(n->clone());
	}

	ret.width = width;
	ret.out_nodes_offset = out_nodes_offset;

	ret.finalize_nodes();

	return ret;
}
