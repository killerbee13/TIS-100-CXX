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
			if (in->filename.empty()) {
				append(ret, "[");
				for (auto v : in->inputs) {
					append(ret, v, ", ");
				}
				append(ret, "]\n");
			} else {
				append(ret, in->filename, '\n');
			}
		} else if (p->type() == node::out) {
			auto on = static_cast<output_node*>(p);
			append(ret, 'O', p->x, " LIST ");
			if (on->filename.empty()) {
				append(ret, "[");
				for (auto v : on->outputs_expected) {
					append(ret, v, ", ");
				}
				append(ret, "]\n");
			} else {
				append(ret, on->filename);
				if (on->d != ' ') {
					append(ret, ' ', +on->d);
				}
				append(ret, '\n');
			}
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
