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

#include "parser.hpp"
#include "T21.hpp"
#include "T30.hpp"
#include "io.hpp"

#include <kblib/convert.h>
#include <kblib/io.h>
#include <kblib/simple.h>
#include <kblib/stringops.h>

#include <spanstream>

template <typename Node>
Node* do_insert(std::vector<std::unique_ptr<node>>& v, int x) {
	auto _ = std::make_unique<Node>();
	auto p = _.get();
	p->x = x;
	v.emplace_back(std::move(_));
	return p;
}

field parse_layout(std::string_view layout) {
	auto ss
	    = std::ispanstream{std::span<const char>{layout.begin(), layout.size()}};

	int h{};
	int w{};
	if (not (ss >> h >> std::ws >> w)) {
		throw std::invalid_argument{""};
	}
	field ret;
	ret.io_node_offset = w * h;
	ret.nodes.resize(ret.io_node_offset);
	ret.width = w;

	for (const int i : kblib::range(w * h)) {
		char c{};
		ss >> c;
		switch (c) {
		case 'C':
			ret.nodes[i] = std::make_unique<T21>();
			break;
		case 'S':
		case 'M':
			ret.nodes[i] = std::make_unique<T30>();
			break;
		default:
			// warn
			[[fallthrough]];
		case 'D':
			ret.nodes[i] = std::make_unique<damaged>();
			break;
		case std::char_traits<char>::eof():
			throw std::invalid_argument{""};
		}
	}
	for (const auto y : kblib::range(ret.height())) {
		for (const auto x : kblib::range(ret.width)) {
			auto p = ret.node_by_location(x, y);
			// safe because node_by_location returns nullptr on OOB
			p->neighbors[up] = ret.node_by_location(x, y - 1);
			p->neighbors[left] = ret.node_by_location(x - 1, y);
			p->neighbors[right] = ret.node_by_location(x + 1, y);
			p->neighbors[down] = ret.node_by_location(x, y + 1);
		}
	}

	std::string id;
	while (ss >> id) {
		if (id.empty()) {
			continue;
		}
		if (id[0] == 'I') {
			auto x = kblib::parse_integer<int>(std::string_view{id}.substr(1), 10);
			auto p = do_insert<input_node>(ret.nodes, x);
			auto n = ret.node_by_location(x, 0);
			assert(n);
			p->neighbors[down] = n;
			n->neighbors[up] = p;
			std::string input_type;
			ss >> input_type;
			if (input_type == "NUMERIC") {
				ss >> p->filename;
				p->io_type = numeric;
			} else if (input_type == "ASCII") {
				ss >> p->filename;
				p->io_type = ascii;
			} else if (input_type == "LIST") {
				p->io_type = list;
				// not yet implemented
				throw 0;
			}
		} else if (id[0] == 'O') {
			auto x = kblib::parse_integer<int>(std::string_view{id}.substr(1), 10);
			std::string input_type;
			ss >> input_type;
			if (input_type == "IMAGE") {
				auto p = do_insert<image_output>(ret.nodes, x);
				auto n = ret.node_by_location(x, ret.height() - 1);
				assert(n);
				p->neighbors[up] = n;
				n->neighbors[down] = p;
				ss >> p->width;
			} else {
				auto p = do_insert<output_node>(ret.nodes, x);
				auto n = ret.node_by_location(x, ret.height() - 1);
				assert(n);
				p->neighbors[up] = n;
				n->neighbors[down] = p;
				if (input_type == "NUMERIC") {
					ss >> p->filename;
					p->io_type = numeric;
					ss >> std::ws;
					auto c = ss.peek();
					if (c >= '0' and c <= '9') {
						int d{};
						ss >> d;
						p->d = static_cast<char>(d);
					}
				} else if (input_type == "ASCII") {
					ss >> p->filename;
					p->io_type = ascii;
				} else if (input_type == "LIST") {
					p->io_type = list;
				}
			}
		} else {
			throw std::invalid_argument{""};
		}
	}

	return ret;
}

field parse(std::string_view layout, std::string_view source,
            std::string_view expected, int T21_size, int T30_size) {
	field ret = parse_layout(layout);
	return ret;
}

std::size_t field::instructions() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (p and p->type() == node::T21) {
			ret += static_cast<T21*>(p.get())->code.size();
		}
	}
	return ret;
}

std::size_t field::nodes_used() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (p and p->type() == node::T21) {
			ret += not static_cast<T21*>(p.get())->code.empty();
		}
	}
	return ret;
}

std::string field::layout() const {

	auto h = height();
	std::string ret = kblib::concat(h, ' ', width, '\n');
	for (const auto y : kblib::range(h)) {
		for (const auto x : kblib::range(width)) {
			auto p = node_by_location(x, y);
			if (not p or p->type() == node::Damaged) {
				ret += 'D';
			} else if (p->type() == node::T21) {
				ret += 'C';
			} else if (p->type() == node::T30) {
				ret += 'S';
			}
		}
		ret += '\n';
	}

	for (const auto i : kblib::range(io_node_offset, nodes.size())) {
		auto p = nodes[i].get();

		constexpr std::array<std::string_view, 3> io_labels{" NUMERIC ",
		                                                    " ASCII ", " LIST "};

		if (not p) {
			continue;
		} else if (p->type() == node::in) {
			auto in = static_cast<input_node*>(p);
			if (in->filename.empty()) {
				kblib::append(ret, 'I', p->x, io_labels[in->io_type], "[");
				for (auto v : in->inputs) {
					kblib::append(ret, v, ", ");
				}
				kblib::append(ret, "]\n");
			} else {
				kblib::append(ret, 'I', p->x, io_labels[in->io_type], in->filename,
				              '\n');
			}
		} else if (p->type() == node::out) {
			auto on = static_cast<output_node*>(p);
			if (on->filename.empty()) {
				kblib::append(ret, 'O', p->x, io_labels[on->io_type], "[");
				for (auto v : on->outputs_expected) {
					kblib::append(ret, v, ", ");
				}
				kblib::append(ret, "]\n");
			} else {
				kblib::append(ret, 'O', p->x, io_labels[on->io_type], on->filename);
				if (on->d != ' ') {
					kblib::append(ret, ' ', +on->d);
				}
				kblib::append(ret, '\n');
			}
		} else if (p->type() == node::image) {
			auto im = static_cast<image_output*>(p);
			kblib::append(ret, 'O', p->x, " IMAGE ", im->width, im->height(),
			              '\n');
		}
	}

	return ret;
}
