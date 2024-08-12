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

#include "parser.hpp"
#include "T21.hpp"
#include "T30.hpp"
#include "io.hpp"

#include <kblib/convert.h>
#include <kblib/stringops.h>

#include <set>

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
	in_nodes_offset = 12;
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
	for (const auto y : range(3u)) {
		for (const auto x : range(4u)) {
			if (auto p = node_by_location(x, y); valid(p)) {
				// safe because node_by_location returns nullptr on OOB
				p->neighbors[left] = node_by_location(x - 1, y);
				p->neighbors[right] = node_by_location(x + 1, y);
				p->neighbors[up] = node_by_location(x, y - 1);
				p->neighbors[down] = node_by_location(x, y + 1);
				for (auto& n : p->neighbors) {
					if (n and n->type() == node::Damaged) {
						n = nullptr;
					}
				}
			}
		}
	}

	out_nodes_offset = in_nodes_offset;
	for (const auto x : range(4u)) {
		auto in = spec.io[0][x];
		switch (in.type) {
		case node::in: {
			auto p = do_insert<input_node>(nodes, to_signed(x));
			auto n = node_by_location(x, 0);
			assert(valid(n));
			p->neighbors[down] = n;
			n->neighbors[up] = p;
			p->filename = concat('@', x);
			out_nodes_offset++;
		} break;
		case node::null:
			// pass
			break;
		default:
			throw std::invalid_argument{"invalid layout spec: illegal input node"};
		}
	}

	for (const auto x : range(4u)) {
		auto in = spec.io[1][x];
		switch (in.type) {
		case node::out: {
			auto p = do_insert<output_node>(nodes, to_signed(x));
			auto n = node_by_location(x, 2);
			assert(valid(n));
			p->neighbors[up] = n;
			n->neighbors[down] = p;
			p->filename = concat('@', 4 + x);
			if (in.image_size) {
				throw std::invalid_argument{"invalid layout spec: image size "
				                            "specified for numeric output node"};
			}
		} break;
		case node::image: {
			auto p = do_insert<image_output>(nodes, to_signed(x));
			auto n = node_by_location(x, 2);
			assert(valid(n));
			p->neighbors[up] = n;
			n->neighbors[down] = p;
			p->filename = concat('@', 4 + x);
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
	auto ss = std::istringstream{std::string(layout)};

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
	ret.in_nodes_offset = static_cast<std::size_t>(w * h);
	ret.nodes.resize(ret.in_nodes_offset);
	ret.width = static_cast<std::size_t>(w);

	{
		int x{};
		int y{};
		for (const auto i : range(ret.in_nodes_offset)) {
			char c{};
			ss >> std::ws >> c;
			switch (c) {
			case cs[0]:
				ret.nodes[i] = std::make_unique<T21>(x, y);
				break;
				// S (stack) and M (memory) are interchangeable
			case cs[1]:
			case cs[2]: {
				ret.nodes[i] = std::make_unique<T30>(x, y, T30_size);
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
	for (const auto y : range(ret.height())) {
		for (const auto x : range(ret.width)) {
			if (auto p = ret.node_by_location(x, y); valid(p)) {
				// safe because node_by_location returns nullptr on OOB
				p->neighbors[left] = ret.node_by_location(x - 1, y);
				p->neighbors[right] = ret.node_by_location(x + 1, y);
				p->neighbors[up] = ret.node_by_location(x, y - 1);
				p->neighbors[down] = ret.node_by_location(x, y + 1);
				for (auto& n : p->neighbors) {
					if (n and n->type() == node::Damaged) {
						n = nullptr;
					}
				}
			}
		}
	}

	ret.out_nodes_offset = ret.in_nodes_offset;
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
			} else if (input_type == "ASCII") {
				ss >> p->filename;
			} else if (input_type == "LIST") {
				// not yet implemented
				throw 0;
			}
			ret.out_nodes_offset++; // we assume all I nodes come before O nodes
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
					ss >> std::ws;
					auto c = ss.peek();
					if (c >= '0' and c <= '9') {
						int d{};
						ss >> d;
						p->d = static_cast<char>(d);
					}
				} else if (output_type == "ASCII") {
					ss >> p->filename;
				} else if (output_type == "LIST") {
				}
			}
		} else {
			throw std::invalid_argument{
			    concat("invalid layout id=", kblib::quoted(id))};
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
			throw std::invalid_argument{concat("duplicate node label ", i)};
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
			throw std::invalid_argument{concat("node label ", i, " out of range")};
		}
		auto code = assemble(section, i, T21_size);
		if (code.size() <= p->small_.size()) {
			p->code = {p->small_.begin(),
			           std::copy(code.begin(), code.end(), p->small_.begin())};
		} else {
			p->large_.reset(new instr[code.size()]);
			p->code
			    = {p->large_.get(), std::ranges::copy(code, p->large_.get()).out};
		}
	}
}

void set_expected(field& f, const single_test& expected) {
	std::size_t in_idx{};
	std::size_t out_idx{};
	for (auto it = f.begin(); it != f.end(); ++it) {
		auto p = it->get();
		p->reset();
		log_debug("reset node (", p->x, ',', p->y, ')');

		if (p->type() == node::in) {
			assert(in_idx < expected.inputs.size());
			auto i = static_cast<input_node*>(p);
			i->inputs = expected.inputs[in_idx++];
			auto log = log_debug();
			log << "set expected input I" << i->x << ":";
			write_list(log, i->inputs);
		} else if (p->type() == node::out) {
			assert(out_idx < expected.n_outputs.size());
			auto o = static_cast<output_node*>(p);
			o->outputs_expected = expected.n_outputs[out_idx++];
			o->complete = o->outputs_expected.empty();
			auto log = log_debug();
			log << "set expected output O" << o->x << ":";
			write_list(log, o->outputs_expected);
		} else if (p->type() == node::image) {
			auto i = static_cast<image_output*>(p);
			i->image_expected = expected.i_output;
			auto log = log_debug();
			log << "set expected image O" << i->x << ": {\n";
			log.log_r([&] { return i->image_expected.write_text(color_logs); });
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
		throw std::invalid_argument{concat("invalid port number ", etoi(p))};
	}
}

std::string to_string(instr i) {
	using enum instr::op;
	if (i.op_ <= neg) {
		return to_string(i.op_);
	} else if (i.op_ == mov) {
		if (i.src == immediate) {
			return concat(to_string(i.op_), ' ', i.val, ',', to_string(i.dst()));
		} else {
			return concat(to_string(i.op_), ' ', to_string(i.src), ',',
			              to_string(i.dst()));
		}
	} else if (i.op_ <= sub) {
		if (i.src == immediate) {
			return concat(to_string(i.op_), ' ', std::to_string(i.val));
		} else {
			return concat(to_string(i.op_), ' ', to_string(i.src));
		}
	} else if (i.op_ <= jlz) {
		return concat(to_string(i.op_), ' ', 'L', i.data);
	} else if (i.op_ == jro) {
		if (i.src == immediate) {
			return concat(to_string(i.op_), ' ', std::to_string(i.val));
		} else {
			return concat(to_string(i.op_), ' ', to_string(i.src));
		}
	} else {
		throw std::invalid_argument{"Unknown instruction "
		                            + std::to_string(i.op_)};
	}
}

instr::op parse_op(std::string_view str) {
	using enum instr::op;
	if (str == "HCF") {
		return hcf;
	} else if (str == "NOP") {
		return nop;
	} else if (str == "SWP") {
		return swp;
	} else if (str == "SAV") {
		return sav;
	} else if (str == "NEG") {
		return neg;
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

// This is a bit more lax than the game, in accepting L, R, U, and D
// abbreviations
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
		throw std::invalid_argument{kblib::quoted(str)
		                            + " is not a valid port or register name"};
	}
}

std::pair<port, word_t> parse_port_or_immediate(const std::string& token) {
	std::pair<port, word_t> ret{};
	if ("-0123456789"sv.contains(token.front())) {
		ret.first = port::immediate;
		ret.second = kblib::parse_integer<word_t>(token);
		if (ret.second < word_min or ret.second > word_max) {
			throw std::invalid_argument{
			    concat("Immediate value ", ret.second, " out of range -999:999")};
		}
	} else {
		ret.first = parse_port(token);
	}
	return ret;
}

void push_label(std::string_view lab, int l,
                std::vector<std::pair<std::string, word_t>>& labels) {
	for (const auto& o : labels) {
		if (o.first == lab) {
			throw std::invalid_argument{
			    concat("Label ", kblib::quoted(lab), " defined multiple times")};
		}
	}
	log_debug("L: ", lab, " (", l, ")");
	labels.emplace_back(lab, l);
}

word_t parse_label(std::string_view label,
                   const std::vector<std::pair<std::string, word_t>>& labels) {
	for (const auto& lab : labels) {
		if (lab.first == label) {
			return lab.second;
		}
	}
	throw std::invalid_argument{
	    concat("Label ", kblib::quoted(label), " used but not defined")};
}

std::vector<instr> assemble(std::string_view source, int node,
                            std::size_t T21_size) {
	auto lines = kblib::split_dsv(source, '\n');
	if (lines.size() > T21_size) {
		throw std::invalid_argument{concat("too many lines of asm for node ",
		                                   node, "; ", lines.size(),
		                                   " exceeds limit ", T21_size)};
	}
	std::vector<instr> ret;
	std::vector<std::pair<std::string, word_t>> labels;

	int l{};
	for (auto& line : lines) {
		// Apparently the game allows ! anywhere as long as there's only one
		if (auto bang = line.find_first_of('!'); bang != std::string::npos) {
			line[bang] = ' ';
		}
		for (auto c : line) {
			// the game won't let you type '`' or '\t' but (sort of) handles
			// them in saves. Same with '@' not followed by a digit but that
			// breaks the logic
			if (c == '@' or (c < ' ' and c != '\t') or c > '~') {
				throw std::invalid_argument{concat(
				    '@', node, ':', l, ": Invalid assembly ", kblib::quoted(line),
				    ", character ", kblib::escapify(c), " not allowed in source")};
			}
		}
		auto tokens
		    = kblib::split_tokens(line.substr(0, line.find_first_of('#')),
		                          [](char c) { return " \t,"sv.contains(c); });
		for (auto j : range(tokens.size())) {
			const auto& tok = tokens[j];
			if (tok.empty()) {
				continue; // ?
			}
			std::string tmp;
			for (auto c : tok) {
				if (c == ':') {
					if (tmp.empty()) {
						throw std::invalid_argument{
						    concat('@', node, ':', l, ": Invalid label \"\"")};
					}
					push_label(tmp, l, labels);
					tmp.clear();
				} else if (not " \t"sv.contains(c)) {
					tmp.push_back(c);
				} else {
					break;
				}
			}
			if (not tmp.empty()) {
				++l;
				break;
			}
		}
	}

	l = 0;
	for (const auto& line : lines) {
		bool seen_op{false};
		auto tokens
		    = kblib::split_tokens(line.substr(0, line.find_first_of('#')),
		                          [](char c) { return " \t,"sv.contains(c); });
		std::optional<instr> tmp;
		// remove labels
		for (auto j : range(tokens.size())) {
			auto& tok = tokens[j];
			if (tok.contains(':')) {
				if (seen_op) {
					throw std::invalid_argument{concat(
					    '@', node, ':', l, ": Labels must be first on a line")};
				}
				tok = tok.substr(tok.find_last_of(':') + 1);
			}
			if (tok.empty()) {
				continue;
			} else {
				seen_op = true;
			}
		}
		std::erase(tokens, "");

		auto assert_last_operand = [&](std::size_t j) {
			if (tokens.size() < j + 1) {
				throw std::invalid_argument{
				    concat('@', node, ':', l, ": Expected operand")};
			}
			if (tokens.size() > j + 1) {
				throw std::invalid_argument{concat('@', node, ':', l,
				                                   ": Unexpected operand ",
				                                   kblib::quoted(tokens[j + 1]))};
			}
		};
		for (auto j : range(tokens.size())) {
			auto& tok = tokens[j];
			if (tok.empty()) {
				continue;
			}
			auto& i = tmp.emplace();
			using enum instr::op;
			if (tok == "HCF") {
				i.op_ = hcf;
				assert_last_operand(j);
			} else if (tok == "NOP") {
				i.op_ = nop;
				assert_last_operand(j);
			} else if (tok == "SWP") {
				i.op_ = swp;
				assert_last_operand(j);
			} else if (tok == "SAV") {
				i.op_ = sav;
				assert_last_operand(j);
			} else if (tok == "NEG") {
				i.op_ = neg;
				assert_last_operand(j);
			} else if (tok == "MOV") {
				i.op_ = mov;
				assert_last_operand(j + 2);
				auto r = parse_port_or_immediate(tokens[j + 1]);
				i.src = r.first;
				i.val = r.second;
				i.data = parse_port(tokens[j + 2]);
			} else if (tok == "ADD") {
				i.op_ = add;
				assert_last_operand(j + 1);
				auto r = parse_port_or_immediate(tokens[j + 1]);
				i.src = r.first;
				i.val = r.second;
			} else if (tok == "SUB") {
				i.op_ = sub;
				assert_last_operand(j + 1);
				auto r = parse_port_or_immediate(tokens[j + 1]);
				i.src = r.first;
				i.val = r.second;
			} else if (tok == "JMP") {
				i.op_ = jmp;
				assert_last_operand(j + 1);
				i.data = parse_label(tokens[j + 1], labels);
			} else if (tok == "JEZ") {
				i.op_ = jez;
				assert_last_operand(j + 1);
				i.data = parse_label(tokens[j + 1], labels);
			} else if (tok == "JNZ") {
				i.op_ = jnz;
				assert_last_operand(j + 1);
				i.data = parse_label(tokens[j + 1], labels);
			} else if (tok == "JGZ") {
				i.op_ = jgz;
				assert_last_operand(j + 1);
				i.data = parse_label(tokens[j + 1], labels);
			} else if (tok == "JLZ") {
				i.op_ = jlz;
				assert_last_operand(j + 1);
				i.data = parse_label(tokens[j + 1], labels);
			} else if (tok == "JRO") {
				i.op_ = jro;
				assert_last_operand(j + 1);
				auto r = parse_port_or_immediate(tokens[j + 1]);
				i.src = r.first;
				i.val = r.second;
			} else {
				throw std::invalid_argument{
				    concat('@', node, ':', l, ": ", kblib::quoted(tok),
				           " is not a valid instruction opcode")};
			}
			log_debug_r([&] { return "parsed: " + to_string(i); });
			ret.push_back(i);
			tmp.reset();
			break;
		}
		++l;
	}

	// normalize labels at the end of the code
	for (auto& i : ret) {
		if (i.op_ >= instr::jmp and i.op_ <= instr::jlz) {
			if (std::cmp_greater_equal(i.data, ret.size())) {
				log_debug("Normalized label ", i.data, "/", ret.size(), "->0");
				i.data = 0;
			}
		}
	}
	return ret;
}
