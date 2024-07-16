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
#include <kblib/variant.h>

#include <spanstream>

using namespace std::literals;

template <typename Node>
Node* do_insert(std::vector<std::unique_ptr<node>>& v, int x) {
	auto _ = std::make_unique<Node>();
	auto p = _.get();
	p->x = x;
	v.emplace_back(std::move(_));
	return p;
}

template <bool use_nonstandard_rep = false>
field parse_layout(std::string_view layout) {
	auto ss
	    = std::ispanstream{std::span<const char>{layout.begin(), layout.size()}};

	constexpr static std::string_view cs = use_nonstandard_rep ? "BSMC" : "CSMD";

	int h{};
	int w{};
	// if no height and width set, assume "3 4"
	if ("BCDMS"sv.contains(ss.peek())) {
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

	for (const auto i : kblib::range(ret.io_node_offset)) {
		char c{};
		ss >> std::ws >> c;
		switch (c) {
		case cs[0]:
			ret.nodes[i] = std::make_unique<T21>();
			break;
			// S (stack) and M (memory) are interchangeable
		case cs[1]:
		case cs[2]:
			ret.nodes[i] = std::make_unique<T30>();
			break;
		default:
			std::clog << "unknown node type " << kblib::quoted(c)
			          << " in layout\n";
			[[fallthrough]];
		case cs[3]:
			ret.nodes[i] = std::make_unique<damaged>();
			break;
		case std::char_traits<char>::eof():
			throw std::invalid_argument{""};
		}
	}
	for (const auto y : kblib::range(ret.height())) {
		for (const auto x : kblib::range(ret.width)) {
			if (auto p = ret.node_by_location(x, y); valid(p)) {
				// safe because node_by_location returns nullptr on OOB
				p->neighbors[up] = ret.node_by_location(x, y - 1);
				p->neighbors[left] = ret.node_by_location(x - 1, y);
				p->neighbors[right] = ret.node_by_location(x + 1, y);
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
			std::string input_type;
			ss >> input_type;
			if (input_type == "IMAGE") {
				auto p = do_insert<image_output>(ret.nodes, x);
				auto n = ret.node_by_location(static_cast<std::size_t>(x),
				                              ret.height() - 1);
				assert(valid(n));
				p->neighbors[up] = n;
				n->neighbors[down] = p;
				ss >> p->width;
			} else {
				auto p = do_insert<output_node>(ret.nodes, x);
				auto n = ret.node_by_location(static_cast<std::size_t>(x),
				                              ret.height() - 1);
				assert(valid(n));
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
			throw std::invalid_argument{kblib::concat("invalid layout")};
		}
	}

	return ret;
}

field parse_layout_guess(std::string_view layout) {
	/*std::clog << "layout detection: " << layout.find_first_of('B') << " < "
	          << layout.find_first_of("IO") << ": " << std::boolalpha
	          << (layout.find_first_of('B') < layout.find_first_of("IO"))
	          << '\n';//*/
	if (layout.find_first_of('B') < layout.find_first_of("IO")) {
		return parse_layout<true>(layout);
	} else {
		return parse_layout<false>(layout);
	}
}

void parse_code(field& f, std::string_view source, int T21_size);

field parse(std::string_view layout, std::string_view source,
            std::string_view expected, int T21_size, int T30_size) {
	field ret = parse_layout_guess(layout);

	return ret;
}

std::string to_string(port p) {
	switch (p) {
	case up:
		return "UP";
	case left:
		return "LEFT";
	case right:
		return "RIGHT";
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
	return kblib::visit_indexed(
	    i.data,
	    [](kblib::constant<std::size_t, instr::nop>, seq_instr) {
		    return "NOP"s;
	    },
	    [](kblib::constant<std::size_t, instr::swp>, seq_instr) {
		    return "SWP"s;
	    },
	    [](kblib::constant<std::size_t, instr::sav>, seq_instr) {
		    return "SAV"s;
	    },
	    [](kblib::constant<std::size_t, instr::neg>, seq_instr) {
		    return "NEG"s;
	    },
	    [](kblib::constant<std::size_t, instr::hcf>, seq_instr) {
		    return "HCF"s;
	    },
	    [](kblib::constant<std::size_t, instr::mov>, mov_instr i) {
		    return "MOV "s + to_string(i);
	    },
	    [](kblib::constant<std::size_t, instr::add>, arith_instr i) {
		    return "ADD "s + to_string(i);
	    },
	    [](kblib::constant<std::size_t, instr::sub>, arith_instr i) {
		    return "SUB "s + to_string(i);
	    },
	    [](kblib::constant<std::size_t, instr::jmp>, jmp_instr i) {
		    return "JMP "s + to_string(i);
	    },
	    [](kblib::constant<std::size_t, instr::jez>, jmp_instr i) {
		    return "JEZ "s + to_string(i);
	    },
	    [](kblib::constant<std::size_t, instr::jnz>, jmp_instr i) {
		    return "JNZ "s + to_string(i);
	    },
	    [](kblib::constant<std::size_t, instr::jgz>, jmp_instr i) {
		    return "JGZ "s + to_string(i);
	    },
	    [](kblib::constant<std::size_t, instr::jlz>, jmp_instr i) {
		    return "JLZ "s + to_string(i);
	    },
	    [](kblib::constant<std::size_t, instr::jro>, jro_instr i) {
		    return "JRO "s + to_string(i);
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
	if (str == "UP" or str == "U") {
		return up;
	} else if (str == "LEFT" or str == "L") {
		return left;
	} else if (str == "RIGHT" or str == "R") {
		return right;
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
	std::cerr << "L: " << lab << " (" << l << ")\n";
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

std::vector<instr> assemble(std::string_view source, int T21_size) {
	auto lines = kblib::split_dsv(source, '\n');
	if (std::cmp_greater(lines.size(), T21_size)) {
		throw std::invalid_argument{""};
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
			std::cerr << "parsed: " << to_string(i) << '\n';
			ret.push_back(i);
			tmp.reset();
			break;
		}
	}
	return ret;
}

std::size_t field::instructions() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (p and type(p.get()) == node::T21) {
			ret += static_cast<T21*>(p.get())->code.size();
		}
	}
	return ret;
}

std::size_t field::nodes_used() const {
	std::size_t ret{};
	for (auto& p : nodes) {
		if (p and type(p.get()) == node::T21) {
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
