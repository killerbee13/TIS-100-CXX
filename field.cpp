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

#include "field.hpp"
#include "T30.hpp"

void field::set_neighbors() {
	for (const auto y : range(height())) {
		for (const auto x : range(width)) {
			if (auto p = node_by_location(x, y); valid(p)) {
				// safe because node_by_location returns nullptr on OOB
				p->neighbors[left] = node_by_location(x - 1, y);
				p->neighbors[right] = node_by_location(x + 1, y);
				p->neighbors[up] = node_by_location(x, y - 1);
				p->neighbors[down] = node_by_location(x, y + 1);
				for (auto& n : p->neighbors) {
					if (not valid(n)) {
						n = nullptr;
					}
				}
			}
		}
	}

	for (const auto x : range(in_nodes_offset, out_nodes_offset)) {
		auto p = nodes[x].get();
		assert(valid(p));
		auto n = node_by_location(p->x, 0);
		assert(valid(n));
		p->neighbors[down] = n;
		n->neighbors[up] = p;
	}

	for (const auto x : range(out_nodes_offset, nodes.size())) {
		auto p = nodes[x].get();
		assert(valid(p));
		auto n = node_by_location(p->x, height() - 1);
		assert(valid(n));
		p->neighbors[up] = n;
		n->neighbors[down] = p;
	}
}

field::field(builtin_layout_spec spec, std::size_t T30_size) {
	nodes.reserve([&] {
		std::size_t r = 12;
		for (auto t : spec.io) {
			for (auto i : t) {
				if (i.type != node::null) {
					++r;
				}
			}
		}
		return r;
	}());

	width = 4;
	in_nodes_offset = 12;
	out_nodes_offset = in_nodes_offset;
	for (const auto y : range(3u)) {
		for (const auto x : range(4u)) {
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
		switch (in.type) {
		case node::in: {
			nodes.push_back(std::make_unique<input_node>(x, -1));
			out_nodes_offset++;
		} break;
		case node::null:
			// pass
			break;
		default:
			throw std::invalid_argument{"invalid layout spec: illegal input node"};
		}
	}

	for (const auto x : range(static_cast<int>(width))) {
		auto out = spec.io[1][x];
		switch (out.type) {
		case node::out: {
			if (out.image_size) {
				throw std::invalid_argument{"invalid layout spec: image size "
				                            "specified for numeric output node"};
			}
			nodes.push_back(std::make_unique<output_node>(x, height()));
		} break;
		case node::image: {
			if (not out.image_size) {
				throw std::invalid_argument{
				    "invalid layout spec: no size specified for image"};
			}
			auto p = std::make_unique<image_output>(x, height());
			p->reshape(out.image_size.value().first,
			           out.image_size.value().second);
			nodes.emplace_back(std::move(p));
		} break;
		case node::null:
			// pass
			break;
		default:
			throw std::invalid_argument{
			    "invalid layout spec: illegal output node"};
		}
	}

	set_neighbors();
}

std::size_t field::instructions() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (p.get()->type() == node::T21) {
			ret += static_cast<T21*>(p.get())->code.size();
		}
	}
	return ret;
}

std::size_t field::nodes_used() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (p.get()->type() == node::T21) {
			ret += not static_cast<T21*>(p.get())->code.empty();
		}
	}
	return ret;
}
std::size_t field::nodes_avail() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (p.get()->type() == node::T21) {
			ret += 1;
		}
	}
	return ret;
}

std::string field::layout() const {

	auto h = height();
	std::string ret = concat(h, ' ', width, '\n');
	for (const auto y : range(h)) {
		for (const auto x : range(width)) {
			auto p = node_by_location(x, y);
			if (not valid(p)) {
				ret += 'D';
			} else if (p->type() == node::T21) {
				ret += 'C';
			} else if (p->type() == node::T30) {
				ret += 'S';
			}
		}
		ret += '\n';
	}

	for (auto it = begin_io(); it != end(); ++it) {
		auto p = it->get();

		if (p->type() == node::in) {
			auto in = static_cast<input_node*>(p);
			append(ret, 'I', p->x, " LIST ");
			append(ret, "[");
			for (auto v : in->inputs) {
				append(ret, v, ", ");
			}
			append(ret, "]\n");
		} else if (p->type() == node::out) {
			auto on = static_cast<output_node*>(p);
			append(ret, 'O', p->x, " LIST ");
			append(ret, "[");
			for (auto v : on->outputs_expected) {
				append(ret, v, ", ");
			}
			append(ret, "]\n");
		} else if (p->type() == node::image) {
			auto im = static_cast<image_output*>(p);
			append(ret, 'O', p->x, " IMAGE ", im->width, im->height);
			if (not im->image_expected.empty()) {
			}
		}
	}

	return ret;
}

constexpr std::string type_name(node::type_t t) {
	switch (t) {
	case node::T21:
		return "T21";
	case node::T30:
		return "T30";
	case node::in:
		return "in";
	case node::out:
		return "out";
	case node::image:
		return "image";
	case node::Damaged:
		return "Damaged";
	default:
		return "null";
	}
}

std::string field::machine_layout() const {
	std::ostringstream ret;
	ret << "{.nodes = {{\n";
	{
		auto it = begin();
		for (auto y = 0; y != 3; ++y) {
			ret << "\t\t\t{";
			for (auto x = 0; x != 4; ++x, ++it) {
				ret << type_name((*it)->type()) << ", ";
			}
			ret << "},\n";
		}
	}
	ret << "\t\t}}, .io = {{\n";
	std::array<std::array<builtin_layout_spec::io_node_spec, 4>, 2> io;
	for (auto it = begin_io(); it != end(); ++it) {
		auto n = it->get();
		if (n->type() == node::in) {
			io[0][static_cast<std::size_t>(n->x)].type = node::in;
		} else if (n->type() == node::out) {
			io[1][static_cast<std::size_t>(n->x)].type = node::out;
		} else if (auto p = dynamic_cast<image_output*>(n)) {
			io[1][static_cast<std::size_t>(p->x)].type = node::image;
			io[1][static_cast<std::size_t>(p->x)].image_size
			    = {p->width, p->height};
		}
	}
	for (auto t : io) {
		ret << "\t\t\t{{";
		for (auto n : t) {
			ret << "{" << type_name(n.type);
			if (n.image_size) {
				ret << ", {{" << n.image_size.value().first << ", "
				    << n.image_size.value().second << "}}";
			} else {
				ret << ", {}";
			}
			ret << "}, ";
		}
		ret << "}},\n";
	}
	ret << "\t}}}";
	return std::move(ret).str();

	using enum node::type_t;
	// clang-format off
	[[maybe_unused]] builtin_layout_spec s =
		{.nodes = {{
			{T21, T21, T21, T21},
			{T21, T21, T21, T21},
			{T21, T21, T21, T21},
		}},
	  .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{out, {}}, {image, {{30, 18}}}, {null, {}}, {null, {}}, }}
	         }}};
}

field field::clone() const
{
	field ret;

	for (auto& n : nodes) {
		ret.nodes.push_back(n->clone());
	}

	ret.width = width;
	ret.in_nodes_offset = in_nodes_offset;
	ret.out_nodes_offset = out_nodes_offset;

	ret.set_neighbors();

	return ret;
}
