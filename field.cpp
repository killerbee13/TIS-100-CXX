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
	for (const auto y : range(height)) {
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

	for (auto& i : nodes_input) {
		auto* p = &i;
		auto n = node_by_location(p->x, 0);
		assert(valid(n));
		p->neighbors[down] = n;
		n->neighbors[up] = p;
	}

	for (auto& i : nodes_output) {
		auto* p = &i;
		auto n = node_by_location(p->x, height - 1);
		assert(valid(n));
		p->neighbors[up] = n;
		n->neighbors[down] = p;
	}
	for (auto& i : nodes_image) {
		auto* p = &i;
		auto n = node_by_location(p->x, height - 1);
		assert(valid(n));
		p->neighbors[up] = n;
		n->neighbors[down] = p;
	}
}

field::field(builtin_layout_spec spec, std::size_t T30_size) {
	nodes_T21.reserve(12);
	nodes_T30.reserve(12);
	nodes_damaged.reserve(12);
	nodes_input.reserve(4);
	nodes_output.reserve(4);
	nodes_image.reserve(4);

	all_nodes.reserve([&] {
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

	height = 3;
	width = 4;
	for (const auto y : range(height)) {
		for (const auto x : range(width)) {
			switch (spec.nodes[y][x]) {
			case node::T21: {
				auto p = &nodes_T21.emplace_back(x, y);
				all_nodes.push_back(p);
				nodes_active.push_back(p);
			} break;
			case node::T30: {
				auto p = &nodes_T30.emplace_back(x, y, T30_size);
				all_nodes.push_back(p);
				nodes_active.push_back(p);
			} break;
			case node::Damaged: {
				auto p = &nodes_damaged.emplace_back(x, y);
				all_nodes.push_back(p);
			} break;
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
			auto p = &nodes_input.emplace_back(x, -1);
			all_nodes.push_back(p);
			nodes_active.push_back(p);
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
			auto p = &nodes_output.emplace_back(x, height);
			all_nodes.push_back(p);
			nodes_active.push_back(p);
			if (out.image_size) {
				throw std::invalid_argument{"invalid layout spec: image size "
				                            "specified for numeric output node"};
			}
		} break;
		case node::image: {
			auto p = &nodes_image.emplace_back(x, height);
			all_nodes.push_back(p);
			nodes_active.push_back(p);
			if (not out.image_size) {
				throw std::invalid_argument{
				    "invalid layout spec: no size specified for image"};
			}
			p->reshape(out.image_size.value().first,
			           out.image_size.value().second);
		} break;
		case node::null:
			// pass
			break;
		default:
			throw std::invalid_argument{"invalid layout spec: illegal input node"};
		}
	}

	set_neighbors();
}

std::size_t field::instructions() const {
	std::size_t ret{};
	for (auto& n : nodes_T21) {
		ret += n.code.size();
	}
	return ret;
}

std::size_t field::nodes_used() const {
	std::size_t ret{};
	for (auto& n : nodes_T21) {
		ret += not n.code.empty();
	}
	return ret;
}

std::string field::layout() const {

	std::string ret = concat(height, ' ', width, '\n');
	for (const auto y : range(height)) {
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

	for (auto& in : nodes_input) {
		append(ret, 'I', in.x, " LIST ");
		append(ret, "[");
		for (auto v : in.inputs) {
			append(ret, v, ", ");
		}
		append(ret, "]\n");
	}
	for (auto& on : nodes_output) {
		append(ret, 'O', on.x, " LIST ");
		append(ret, "[");
		for (auto v : on.outputs_expected) {
			append(ret, v, ", ");
		}
		append(ret, "]\n");
	}
	for (auto& im : nodes_image) {
		append(ret, 'O', im.x, " IMAGE ", im.width, im.height);
		if (not im.image_expected.empty()) {
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
		auto it = all_nodes.begin();
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
	for (auto& in : nodes_input) {
		io[0][static_cast<std::size_t>(in.x)].type = node::in;
	}
	for (auto& on : nodes_output) {
		io[1][static_cast<std::size_t>(on.x)].type = node::out;
	}
	for (auto& im : nodes_image) {
		io[1][static_cast<std::size_t>(im.x)].type = node::image;
		io[1][static_cast<std::size_t>(im.x)].image_size = {im.width, im.height};
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

	// for (auto& n : nodes) { TODO
	// 	ret.nodes.push_back(n->clone());
	// }

	ret.width = width;

	ret.set_neighbors();

	return ret;
}
