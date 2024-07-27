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
#include "builtin_levels.hpp"
#include "io.hpp"

#include <kblib/convert.h>
#include <kblib/io.h>
#include <kblib/simple.h>
#include <kblib/stringops.h>
#include <kblib/variant.h>

#include <set>
#include <spanstream>

using namespace std::literals;

template <typename Node>
Node* do_insert(std::vector<std::unique_ptr<node>>& v, int x) {
	auto _ = std::make_unique<Node>(x, -1);
	auto p = _.get();
	v.emplace_back(std::move(_));
	return p;
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
	io_node_offset = 12;
	for (std::size_t y = 0; y != 3; ++y) {
		for (std::size_t x = 0; x != 4; ++x) {
			switch (spec.nodes[y][x]) {
			case node::T21:
				nodes.push_back(std::make_unique<T21>(x, y));
				break;
			case node::T30: {
				auto p = std::make_unique<T30>(x, y);
				p->max_size = T30_size;
				nodes.push_back(std::move(p));
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
	for (const auto y : kblib::range(3)) {
		for (const auto x : kblib::range(4)) {
			if (auto p = node_by_location(x, y); valid(p)) {
				// safe because node_by_location returns nullptr on OOB
				p->neighbors[left] = node_by_location(x - 1, y);
				p->neighbors[right] = node_by_location(x + 1, y);
				p->neighbors[up] = node_by_location(x, y - 1);
				p->neighbors[down] = node_by_location(x, y + 1);
			}
		}
	}

	for (std::size_t x = 0; x != 4; ++x) {
		auto in = spec.io[0][x];
		switch (in.type) {
		case node::in: {
			auto p = do_insert<input_node>(nodes, x);
			auto n = node_by_location(static_cast<std::size_t>(x), 0);
			assert(valid(n));
			p->neighbors[down] = n;
			n->neighbors[up] = p;
			p->filename = kblib::concat('@', x);
			p->io_type = numeric;
		} break;
		case node::null:
			// pass
			break;
		default:
			throw std::invalid_argument{"invalid layout spec: illegal input node"};
		}
	}

	for (int x = 0; x != 4; ++x) {
		auto in = spec.io[1][x];
		switch (in.type) {
		case node::out: {
			auto p = do_insert<output_node>(nodes, x);
			auto n = node_by_location(static_cast<std::size_t>(x), 2);
			assert(valid(n));
			p->neighbors[up] = n;
			n->neighbors[down] = p;
			p->io_type = numeric;
			p->filename = kblib::concat('@', 4 + x);
			if (in.image_size) {
				throw std::invalid_argument{"invalid layout spec: image size "
				                            "specified for numeric output node"};
			}
		} break;
		case node::image: {
			auto p = do_insert<image_output>(nodes, x);
			auto n = node_by_location(static_cast<std::size_t>(x), 2);
			assert(valid(n));
			p->neighbors[up] = n;
			n->neighbors[down] = p;
			p->filename = kblib::concat('@', 4 + x);
			if (not in.image_size) {
				throw std::invalid_argument{
				    "invalid layout spec: no size specified for image"};
			}
			p->reshape(in.image_size.value().first, in.image_size.value().second);
		} break;
		case node::null:
			// pass
			break;
		default:
			throw std::invalid_argument{"invalid layout spec: illegal input node"};
		}
	}
}

template <bool use_nonstandard_rep>
field parse_layout(std::string_view layout, std::size_t T30_size) {
	auto ss
	    = std::ispanstream{std::span<const char>{layout.begin(), layout.size()}};

	constexpr static std::string_view cs = use_nonstandard_rep ? "BSMC" : "CSMD";

	if (layout.empty()) {
		throw std::invalid_argument{"empty layout is not valid"};
	}

	int h{};
	int w{};
	// if no height and width set, assume "3 4"
	if ("BCDMS"sv.contains(static_cast<char>(ss.peek()))) {
		h = 3;
		w = 4;
	} else if (not (ss >> h >> std::ws >> w)) {
		throw std::invalid_argument{""};
	}
	if (h < 0 or w < 0) {
		throw std::invalid_argument{""};
	}
	field ret;
	ret.io_node_offset = static_cast<std::size_t>(w * h);
	ret.nodes.resize(ret.io_node_offset);
	ret.width = static_cast<std::size_t>(w);

	{
		int x{};
		int y{};
		for (const auto i : kblib::range(ret.io_node_offset)) {
			char c{};
			ss >> std::ws >> c;
			switch (c) {
			case cs[0]:
				ret.nodes[i] = std::make_unique<T21>(x, y);
				break;
				// S (stack) and M (memory) are interchangeable
			case cs[1]:
			case cs[2]: {
				auto p = std::make_unique<T30>(x, y);
				p->max_size = T30_size;
				ret.nodes[i] = std::move(p);
			} break;
			default:
				log_warn("unknown node type ", kblib::quoted(c), " in layout");
				[[fallthrough]];
			case cs[3]:
				ret.nodes[i] = std::make_unique<damaged>(x, y);
				break;
			case std::char_traits<char>::eof():
				throw std::invalid_argument{""};
			}
			++x;
			if (x == w) {
				x = 0;
				++y;
			}
		}
	}
	for (const auto y : kblib::range(ret.height())) {
		for (const auto x : kblib::range(ret.width)) {
			if (auto p = ret.node_by_location(x, y); valid(p)) {
				// safe because node_by_location returns nullptr on OOB
				p->neighbors[left] = ret.node_by_location(x - 1, y);
				p->neighbors[right] = ret.node_by_location(x + 1, y);
				p->neighbors[up] = ret.node_by_location(x, y - 1);
				p->neighbors[down] = ret.node_by_location(x, y + 1);
			}
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
			auto n = ret.node_by_location(static_cast<std::size_t>(x), 0);
			assert(valid(n));
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
			std::string output_type;
			ss >> output_type;
			if (output_type == "IMAGE") {
				auto p = do_insert<image_output>(ret.nodes, x);
				auto n = ret.node_by_location(static_cast<std::size_t>(x),
				                              ret.height() - 1);
				assert(valid(n));
				p->neighbors[up] = n;
				n->neighbors[down] = p;
				int i_w{};
				int i_h{};
				ss >> p->filename >> i_w >> i_h;
				p->reshape(i_w, i_h);
			} else {
				auto p = do_insert<output_node>(ret.nodes, x);
				auto n = ret.node_by_location(static_cast<std::size_t>(x),
				                              ret.height() - 1);
				assert(valid(n));
				p->neighbors[up] = n;
				n->neighbors[down] = p;
				if (output_type == "NUMERIC") {
					ss >> p->filename;
					p->io_type = numeric;
					ss >> std::ws;
					auto c = ss.peek();
					if (c >= '0' and c <= '9') {
						int d{};
						ss >> d;
						p->d = static_cast<char>(d);
					}
				} else if (output_type == "ASCII") {
					ss >> p->filename;
					p->io_type = ascii;
				} else if (output_type == "LIST") {
					p->io_type = list;
				}
			}
		} else {
			throw std::invalid_argument{
			    kblib::concat("invalid layout id=", kblib::quoted(id))};
		}
	}

	return ret;
}

field parse_layout_guess(std::string_view layout, std::size_t T30_size) {
	/*std::clog << "layout detection: " << layout.find_first_of('B') << " < "
	          << layout.find_first_of("IO") << ": " << std::boolalpha
	          << (layout.find_first_of('B') < layout.find_first_of("IO"))
	          << '\n';//*/
	if (layout.find_first_of('B') < layout.find_first_of("IO")) {
		return parse_layout<true>(layout, T30_size);
	} else {
		return parse_layout<false>(layout, T30_size);
	}
}

std::string_view pop(std::string_view& str, std::size_t n) {
	n = std::min(n, str.size());
	auto r = str.substr(0, n);
	str.remove_prefix(n);
	return r;
}

void parse_code(field& f, std::string_view source, std::size_t T21_size) {
	source.remove_prefix(std::min(source.find_first_of('@'), source.size()));
	std::set<int> nodes_seen;
	while (not source.empty()) {
		auto header = pop(source, source.find_first_of('\n'));
		pop(source, source.find_first_not_of(" \t\r\n"));
		header.remove_prefix(1);
		auto i = kblib::parse_integer<int>(header);
		auto section = pop(source, source.find_first_of('@'));
		if (not nodes_seen.insert(i).second) {
			throw std::invalid_argument{kblib::concat("duplicate node label ", i)};
		}
		if (section.empty()) {
			continue;
		}
		section.remove_suffix(
		    section.size()
		    - std::min(section.find_last_not_of(" \t\r\n"), section.size()) - 1);
		log_debug("index ", i, " of ", f.nodes_avail());
		auto p = f.node_by_index(static_cast<std::size_t>(i));
		if (not p) {
			throw std::invalid_argument{
			    kblib::concat("node label ", i, " out of range")};
		}
		p->code = assemble(section, i, T21_size);
	}
}

void set_expected(field& f, const single_test& expected) {
	auto it = f.begin();
	std::size_t in_idx{};
	std::size_t out_idx{};
	for (; it != f.end(); ++it) {
		auto p = it->get();
		if (p) {
			p->reset();
			log_debug("reset node (", p->x, ',', p->y, ')');
		}
		if (type(p) == node::in) {
			assert(in_idx < expected.inputs.size());
			auto i = static_cast<input_node*>(p);
			i->inputs = expected.inputs[in_idx++];
			auto log = log_debug();
			log << "set expected input I" << i->x << ":";
			write_list(log, i->inputs, nullptr, false);
		} else if (type(p) == node::out) {
			assert(out_idx < expected.n_outputs.size());
			auto o = static_cast<output_node*>(p);
			o->outputs_expected = expected.n_outputs[out_idx++];
			auto log = log_debug();
			log << "set expected output O" << o->x << ":";
			write_list(log, o->outputs_expected, nullptr, false);
		} else if (type(p) == node::image) {
			auto i = static_cast<image_output*>(p);
			i->image_expected = expected.i_output;
			auto log = log_debug();
			log << "set expected image O" << i->x << ": {\n";
			i->image_expected.write_text(log);
			log << '}';
		}
	}
}

std::string to_string(port p) {
	switch (p) {
	case left:
		return "LEFT";
	case right:
		return "RIGHT";
	case up:
		return "UP";
	case down:
		return "DOWN";
	case D5:
		return "D5"; // leaving space for a possible extension to 3d
	case D6:
		return "D6";
	case nil:
		return "NIL";
	case acc:
		return "ACC";
	case any:
		return "ANY";
	case last:
		return "LAST";
	default:
		throw std::invalid_argument{
		    kblib::concat("invalid port number ", kblib::etoi(p))};
	}
}

std::string to_string(seq_instr) { return {}; }
std::string to_string(mov_instr i) {
	if (i.src == immediate) {
		return kblib::concat(i.val, ',', to_string(i.dst));
	} else {
		return kblib::concat(to_string(i.src), ',', to_string(i.dst));
	}
}
std::string to_string(arith_instr i) {
	if (i.src == immediate) {
		return std::to_string(i.val);
	} else {
		return to_string(i.src);
	}
}
std::string to_string(jmp_instr i) { return kblib::concat('L', i.target); }
std::string to_string(jro_instr i) {
	if (i.src == immediate) {
		return std::to_string(i.val);
	} else {
		return to_string(i.src);
	}
}

std::string to_string(instr i) {
	return kblib::visit2(
	    i.data,
	    [&](seq_instr) {
		    return to_string(static_cast<instr::op>(i.data.index()));
	    },
	    [&](auto d) {
		    return kblib::concat(
		        to_string(static_cast<instr::op>(i.data.index())), ' ',
		        to_string(d));
	    });
}

instr::op parse_op(std::string_view str) {
	using enum instr::op;
	if (str == "NOP") {
		return nop;
	} else if (str == "SWP") {
		return swp;
	} else if (str == "SAV") {
		return sav;
	} else if (str == "NEG") {
		return neg;
	} else if (str == "HCF") {
		return hcf;
	} else if (str == "MOV") {
		return mov;
	} else if (str == "ADD") {
		return add;
	} else if (str == "SUB") {
		return sub;
	} else if (str == "JMP") {
		return jmp;
	} else if (str == "JEZ") {
		return jez;
	} else if (str == "JNZ") {
		return jnz;
	} else if (str == "JGZ") {
		return jgz;
	} else if (str == "JLZ") {
		return jlz;
	} else if (str == "JRO") {
		return jro;
	} else {
		throw std::invalid_argument{kblib::quoted(str)
		                            + " is not a valid instruction opcode"};
	}
}

port parse_port(std::string_view str) {
	if (str == "LEFT" or str == "L") {
		return left;
	} else if (str == "RIGHT" or str == "R") {
		return right;
	} else if (str == "UP" or str == "U") {
		return up;
	} else if (str == "DOWN" or str == "D") {
		return down;
	} else if (str == "NIL") {
		return nil;
	} else if (str == "ACC") {
		return acc;
	} else if (str == "ANY") {
		return any;
	} else if (str == "LAST") {
		return last;
	} else {
		throw std::invalid_argument{""};
	}
}

std::pair<port, word_t> parse_port_or_immediate(
    const std::vector<std::string> tokens, std::size_t j) {
	std::pair<port, word_t> ret;
	assert(j < tokens.size());
	if ("-0123456789"sv.contains(tokens[j].front())) {
		ret.first = port::immediate;
		ret.second = kblib::parse_integer<word_t>(tokens[j]);
		if (ret.second < -999 or ret.second > 999) {
			throw std::invalid_argument{""};
		}
	} else {
		ret.first = parse_port(tokens[j]);
	}
	return ret;
}

void push_label(std::string_view lab, int l,
                std::vector<std::pair<std::string, index_t>>& labels) {
	for (const auto& o : labels) {
		if (o.first == lab) {
			throw std::invalid_argument{""};
		}
	}
	log_debug("L: ", lab, " (", l, ")");
	labels.emplace_back(lab, l);
}

index_t parse_label(
    const std::vector<std::string> tokens, std::size_t j,
    const std::vector<std::pair<std::string, index_t>>& labels) {
	assert(j < tokens.size());
	for (const auto& lab : labels) {
		if (lab.first == tokens[j]) {
			return lab.second;
		}
	}
	throw std::invalid_argument{""};
}

std::vector<instr> assemble(std::string_view source, int node,
                            std::size_t T21_size) {
	auto lines = kblib::split_dsv(source, '\n');
	if (lines.size() > T21_size) {
		throw std::invalid_argument{
		    kblib::concat("too many lines of asm for node ", node)};
	}
	std::vector<instr> ret;
	std::vector<std::pair<std::string, index_t>> labels;

	{
		int l{};
		for (const auto& line : lines) {
			auto tokens
			    = kblib::split_tokens(line.substr(0, line.find_first_of('#')),
			                          [](char c) { return " \t,"sv.contains(c); });
			for (auto j : kblib::range(tokens.size())) {
				const auto& tok = tokens[j];
				if (tok.empty()) {
					continue; // ?
				} else if (tok.contains(':')) {
					std::string tmp;
					for (auto c : tok) {
						if (c == ':') {
							if (tmp.empty()) {
								throw std::invalid_argument{""};
							}
							push_label(tmp, l, labels);
							tmp.clear();
						} else if (not " \t"sv.contains(c)) {
							tmp.push_back(c);
						} else {
							break;
						}
					}
				} else {
					++l;
					break;
				}
			}
		}
	}

	/*std::cerr << "labels: ";
	for (auto l : labels) {
	   std::cerr << std::quoted(l.first) << " on instr " << l.second << '\n';
	}
	if (labels.empty()) {
	   std::cerr << '\n';
	}*/

	// int l{};
	for (const auto& line : lines) {
		bool seen_op{false};
		// std::cerr << "l: " << std::quoted(line) << '\n';
		auto tokens
		    = kblib::split_tokens(line.substr(0, line.find_first_of('#')),
		                          [](char c) { return " \t,"sv.contains(c); });
		// std::cerr << "l" << l++ << "(" << tokens.size() << "): ";
		//	for (const auto& t : tokens) {
		//		std::cerr << std::quoted(t) << ' ';
		//	}
		//	std::cerr << '\n';
		std::optional<instr> tmp;
		for (auto j : kblib::range(tokens.size())) {
			auto& tok = tokens[j];
			if (tok.contains(':')) {
				// assert(not tmp);
				assert(not seen_op);
				tok = tok.substr(tok.find_last_of(':') + 1);
			}
			// std::cerr << "t: " << std::quoted(tok) << '\n';
			if (tok.empty()) {
				continue;
			} else {
				seen_op = true;
			}
		}
		std::erase(tokens, "");
		for (auto j : kblib::range(tokens.size())) {
			auto& tok = tokens[j];
			tok.erase(0, tok.find_first_not_of('!'));
			if (tok.empty()) {
				continue;
			}
			auto& i = tmp.emplace();
			using enum instr::op;
			if (tok == "NOP") {
				i.data.emplace<static_cast<std::size_t>(nop)>();
				assert(j + 1 == tokens.size() or tokens[j + 1].starts_with('#'));
			} else if (tok == "SWP") {
				i.data.emplace<static_cast<std::size_t>(swp)>();
				assert(j + 1 == tokens.size() or tokens[j + 1].starts_with('#'));
			} else if (tok == "SAV") {
				i.data.emplace<static_cast<std::size_t>(sav)>();
				assert(j + 1 == tokens.size() or tokens[j + 1].starts_with('#'));
			} else if (tok == "NEG") {
				i.data.emplace<static_cast<std::size_t>(neg)>();
				assert(j + 1 == tokens.size() or tokens[j + 1].starts_with('#'));
			} else if (tok == "HCF") {
				i.data.emplace<static_cast<std::size_t>(hcf)>();
				assert(j + 1 == tokens.size() or tokens[j + 1].starts_with('#'));
			} else if (tok == "MOV") {
				auto& d = i.data.emplace<static_cast<std::size_t>(mov)>();
				assert(j + 2 < tokens.size());
				auto r = parse_port_or_immediate(tokens, j + 1);
				d.src = r.first;
				d.val = r.second;
				d.dst = parse_port(tokens[j + 2]);
				assert(j + 3 == tokens.size() or tokens[j + 3].starts_with('#'));
			} else if (tok == "ADD") {
				auto& d = i.data.emplace<static_cast<std::size_t>(add)>();
				auto r = parse_port_or_immediate(tokens, j + 1);
				d.src = r.first;
				d.val = r.second;
				assert(j + 2 == tokens.size() or tokens[j + 2].starts_with('#'));
			} else if (tok == "SUB") {
				auto& d = i.data.emplace<static_cast<std::size_t>(sub)>();
				auto r = parse_port_or_immediate(tokens, j + 1);
				d.src = r.first;
				d.val = r.second;
				assert(j + 2 == tokens.size() or tokens[j + 2].starts_with('#'));
			} else if (tok == "JMP") {
				auto& d = i.data.emplace<static_cast<std::size_t>(jmp)>();
				d.target = parse_label(tokens, j + 1, labels);
				assert(j + 2 == tokens.size() or tokens[j + 2].starts_with('#'));
			} else if (tok == "JEZ") {
				auto& d = i.data.emplace<static_cast<std::size_t>(jez)>();
				d.target = parse_label(tokens, j + 1, labels);
				assert(j + 2 == tokens.size() or tokens[j + 2].starts_with('#'));
			} else if (tok == "JNZ") {
				auto& d = i.data.emplace<static_cast<std::size_t>(jnz)>();
				d.target = parse_label(tokens, j + 1, labels);
				assert(j + 2 == tokens.size() or tokens[j + 2].starts_with('#'));
			} else if (tok == "JGZ") {
				auto& d = i.data.emplace<static_cast<std::size_t>(jgz)>();
				d.target = parse_label(tokens, j + 1, labels);
				assert(j + 2 == tokens.size() or tokens[j + 2].starts_with('#'));
			} else if (tok == "JLZ") {
				auto& d = i.data.emplace<static_cast<std::size_t>(jlz)>();
				d.target = parse_label(tokens, j + 1, labels);
				assert(j + 2 == tokens.size() or tokens[j + 2].starts_with('#'));
			} else if (tok == "JRO") {
				auto& d = i.data.emplace<static_cast<std::size_t>(jro)>();
				auto r = parse_port_or_immediate(tokens, j + 1);
				d.src = r.first;
				d.val = kblib::saturating_cast<index_t>(r.second);
				assert(j + 2 == tokens.size() or tokens[j + 2].starts_with('#'));
			} else {
				throw std::invalid_argument{kblib::quoted(tok)
				                            + " is not a valid instruction opcode"};
			}
			log_debug_r([&] { return "parsed: " + to_string(i); });
			ret.push_back(i);
			tmp.reset();
			break;
		}
	}

	// normalize labels at the end of the code
	for (auto& i : ret) {
		kblib::visit2(
		    i.data,
		    [&](jmp_instr& j) {
			    if (std::cmp_greater_equal(j.target, ret.size())) {
				    log_debug("Normalized label ", j.target, "/", ret.size(),
				              "->0");
				    j.target = 0;
			    }
		    },
		    [](auto&) {});
	}
	return ret;
}

std::size_t field::instructions() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (type(p.get()) == node::T21) {
			ret += static_cast<T21*>(p.get())->code.size();
		}
	}
	return ret;
}

std::size_t field::nodes_used() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (type(p.get()) == node::T21) {
			ret += not static_cast<T21*>(p.get())->code.empty();
		}
	}
	return ret;
}
std::size_t field::nodes_avail() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (type(p.get()) == node::T21) {
			ret += 1;
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
			if (not valid(p)) {
				ret += 'D';
			} else if (type(p) == node::T21) {
				ret += 'C';
			} else if (type(p) == node::T30) {
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
		} else if (type(p) == node::in) {
			auto in = static_cast<input_node*>(p);
			kblib::append(ret, 'I', p->x, io_labels[in->io_type]);
			if (in->filename.empty()) {
				kblib::append(ret, "[");
				for (auto v : in->inputs) {
					kblib::append(ret, v, ", ");
				}
				kblib::append(ret, "]\n");
			} else {
				kblib::append(ret, in->filename, '\n');
			}
		} else if (type(p) == node::out) {
			auto on = static_cast<output_node*>(p);
			kblib::append(ret, 'O', p->x, io_labels[on->io_type]);
			if (on->filename.empty()) {
				kblib::append(ret, "[");
				for (auto v : on->outputs_expected) {
					kblib::append(ret, v, ", ");
				}
				kblib::append(ret, "]\n");
			} else {
				kblib::append(ret, on->filename);
				if (on->d != ' ') {
					kblib::append(ret, ' ', +on->d);
				}
				kblib::append(ret, '\n');
			}
		} else if (type(p) == node::image) {
			auto im = static_cast<image_output*>(p);
			kblib::append(ret, 'O', p->x, " IMAGE ", im->width, im->height);
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
		break;
	}
}

std::string field::machine_layout() const {
	std::ostringstream ret;
	ret << "{.nodes = {{\n";
	{
		auto it = begin();
		for (auto y = 0; y != 3; ++y) {
			ret << "\t\t{";
			for (auto x = 0; x != 4; ++x, ++it) {
				ret << type_name(type(it->get())) << ", ";
			}
			ret << "},\n";
		}
	}
	ret << "\t}}, .io = {{\n";
	std::array<std::array<builtin_layout_spec::io_node_spec, 4>, 2> io;
	for (auto it = end_regular(); it != end(); ++it) {
		auto n = it->get();
		if (type(n) == node::in) {
			io[0][static_cast<std::size_t>(n->x)].type = node::in;
		} else if (type(n) == node::out) {
			io[1][static_cast<std::size_t>(n->x)].type = node::out;
		} else if (auto p = dynamic_cast<image_output*>(n)) {
			io[1][static_cast<std::size_t>(p->x)].type = node::image;
			io[1][static_cast<std::size_t>(p->x)].image_size
			    = {p->width, p->height};
		}
	}
	for (auto t : io) {
		ret << "\t\t{{\n";
		for (auto n : t) {
			ret << "\t\t\t{.type = " << type_name(n.type);
			if (n.image_size) {
				ret << ", .image_size = {{" << n.image_size.value().first << ", "
				    << n.image_size.value().second << "}}";
			}
			ret << "},\n";
		}
		ret << "\t\t}},\n";
	}
	ret << "\t}}}";
	return std::move(ret).str();

	using enum node::type_t;
	[[maybe_unused]] builtin_layout_spec s = //
	    {.nodes = {{
	         {T21, T21, T21, T21},
	         {T21, T21, T21, T21},
	         {T21, T21, T21, T21},
	     }},
	     .io = {{{{
	                 {.type = null},
	                 {.type = in},
	                 {.type = in},
	                 {.type = null},
	             }},
	             {{
	                 {.type = out},
	                 {.type = image, .image_size = {{30, 18}}},
	                 {.type = null},
	                 {.type = null},
	             }}}}};
}
