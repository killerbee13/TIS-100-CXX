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
#include "instr.hpp"
#include "logger.hpp"

#include <kblib/convert.h>
#include <kblib/stringops.h>

#include <map>

std::string to_string(instr i) {
	using enum instr::op;
	if (i.op_ <= neg) {
		return to_string(i.op_);
	} else if (i.op_ == mov) {
		if (i.src == immediate) {
			return concat(to_string(i.op_), ' ', i.val, ',', port_name(i.dst));
		} else {
			return concat(to_string(i.op_), ' ', port_name(i.src), ',',
			              port_name(i.dst));
		}
	} else if (i.op_ <= sub || i.op_ == jro) {
		if (i.src == immediate) {
			return concat(to_string(i.op_), ' ', i.val);
		} else {
			return concat(to_string(i.op_), ' ', port_name(i.src));
		}
	} else if (i.op_ <= jlz) {
		return concat(to_string(i.op_), ' ', 'L', i.target());
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

// permissive=true allows for any prefix of a valid port to be recognized (i.e.
// L for LEFT, RI for RIGHT, AN for ANY). In case of ambiguity, LEFT and ACC
// have priority
port parse_port(std::string_view str, bool permissive) {
	std::pair<std::string_view, port> ports[]{
	    {"LEFT", left}, {"RIGHT", right}, {"UP", up},   {"DOWN", down},
	    {"NIL", nil},   {"ACC", acc},     {"ANY", any}, {"LAST", last},
	};
	for (auto [tok, val] : ports) {
		if (tok.starts_with(str)) {
			if (not permissive and str != tok) {
				throw std::invalid_argument{concat(
				    "Port abbreviation ", kblib::quoted(str), " is not allowed")};
			}
			return val;
		}
	}
	throw std::invalid_argument{kblib::quoted(str)
	                            + " is not a valid port or register name"};
}

std::vector<instr> assemble(std::string_view source, int node,
                            std::size_t T21_size, bool permissive) {
	auto lines = kblib::split_dsv(source, '\n');
	std::vector<instr> ret;
	std::map<std::string, word_t, std::less<>> labels;

	int l{};
	int noncode_lines{};
	for (auto& line : lines) {
		if (not permissive and line.length() > max_line_length) {
			throw std::invalid_argument{concat('@', node, ':', l, ": Line ",
			                                   kblib::quoted(line), " too long (",
			                                   line.length(), " chars)")};
		}
		// The game allows ! anywhere as long as there's only one per line
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
		if (tokens.empty()) {
			++noncode_lines;
		}
		// the game allows only a single label per line, but multiple labels can
		// still be attached to the same instruction if put in different lines.
		// The --permissive flag allows multiple labels on a single line.
		int label_count{};
		for (const auto& tok : tokens) {
			assert(not tok.empty());
			std::string tmp;
			for (auto c : tok) {
				if (c == ':') {
					if (tmp.empty()) {
						throw std::invalid_argument{
						    concat('@', node, ':', l, ": Invalid label \"\"")};
					}
					if (labels.contains(tmp)) {
						throw std::invalid_argument{
						    concat('@', node, ':', l, ": Label ", kblib::quoted(tmp),
						           " defined multiple times")};
					}
					log_debug("L: ", tmp, " (", l, ")");
					labels[std::exchange(tmp, "")] = to_word(l);
					++label_count;
				} else {
					tmp.push_back(c);
				}
			}
			if (not tmp.empty()) {
				++l;
				break;
			}
		}
		if (not permissive and label_count > 1) {
			throw std::invalid_argument{concat('@', node, ':', l, ": Line ",
			                                   kblib::quoted(line),
			                                   " has too many labels")};
		}
	}

	// Blank lines and lines consisting only of comments don't count with
	// --permissive
	if ((permissive ? (lines.size() - noncode_lines) : lines.size())
	    > T21_size) {
		throw std::invalid_argument{concat("Too many lines of asm for node ",
		                                   node, "; ", lines.size(),
		                                   " exceeds limit ", T21_size)};
	}
	l = 0;
	for (const auto& line : lines) {
		bool seen_op{false};
		auto tokens
		    = kblib::split_tokens(line.substr(0, line.find_first_of('#')),
		                          [](char c) { return " \t,"sv.contains(c); });
		// remove labels
		for (auto& tok : tokens) {
			if (tok.contains(':')) {
				if (seen_op) {
					throw std::invalid_argument{concat(
					    '@', node, ':', l, ": Labels must be first on a line")};
				}
				tok = tok.substr(tok.find_last_of(':') + 1);
			}
			if (not tok.empty()) {
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
		auto parse_label = [&](std::string_view label) -> word_t {
			auto it = labels.find(label);
			if (it != labels.end()) {
				return it->second;
			}
			throw std::invalid_argument{concat('@', node, ':', l, ": Label ",
			                                   kblib::quoted(label),
			                                   " used but not defined")};
		};
		auto load_port_or_immediate = [&](instr& i, const std::string& token) {
			if ("+-0123456789"sv.contains(token.front())) {
				// the game accepts int32 immediates and clamps them to [min, max],
				// the sim enforces the limit in the source directly
				auto immediate = kblib::parse_integer<int32_t>(token, 10);
				if (immediate < word_min or immediate > word_max) {
					throw std::invalid_argument{
					    concat('@', node, ':', l, ": Immediate value ", immediate,
					           " out of range -999:999")};
				}
				i.src = port::immediate;
				i.val = to_word(immediate);
			} else {
				i.src = parse_port(token, permissive);
			}
		};
		if (not tokens.empty()) {
			auto& opcode = tokens[0];
			instr i{};
			using enum instr::op;
			if (opcode == "HCF") {
				i.op_ = hcf;
				assert_last_operand(0);
			} else if (opcode == "NOP") {
				i.op_ = nop;
				assert_last_operand(0);
			} else if (opcode == "SWP") {
				i.op_ = swp;
				assert_last_operand(0);
			} else if (opcode == "SAV") {
				i.op_ = sav;
				assert_last_operand(0);
			} else if (opcode == "NEG") {
				i.op_ = neg;
				assert_last_operand(0);
			} else if (opcode == "MOV") {
				i.op_ = mov;
				assert_last_operand(2);
				load_port_or_immediate(i, tokens[1]);
				i.dst = parse_port(tokens[2], permissive);
			} else if (opcode == "ADD") {
				i.op_ = add;
				assert_last_operand(1);
				load_port_or_immediate(i, tokens[1]);
			} else if (opcode == "SUB") {
				i.op_ = sub;
				assert_last_operand(1);
				load_port_or_immediate(i, tokens[1]);
			} else if (opcode == "JMP") {
				i.op_ = jmp;
				assert_last_operand(1);
				i.val = parse_label(tokens[1]);
			} else if (opcode == "JEZ") {
				i.op_ = jez;
				assert_last_operand(1);
				i.val = parse_label(tokens[1]);
			} else if (opcode == "JNZ") {
				i.op_ = jnz;
				assert_last_operand(1);
				i.val = parse_label(tokens[1]);
			} else if (opcode == "JGZ") {
				i.op_ = jgz;
				assert_last_operand(1);
				i.val = parse_label(tokens[1]);
			} else if (opcode == "JLZ") {
				i.op_ = jlz;
				assert_last_operand(1);
				i.val = parse_label(tokens[1]);
			} else if (opcode == "JRO") {
				i.op_ = jro;
				assert_last_operand(1);
				load_port_or_immediate(i, tokens[1]);
			} else {
				throw std::invalid_argument{
				    concat('@', node, ':', l, ": ", kblib::quoted(opcode),
				           " is not a valid instruction opcode")};
			}
			log_debug_r([&] { return "parsed: " + to_string(i); });
			ret.push_back(i);
		}
		++l;
	}

	// normalize labels at the end of the code
	for (auto& i : ret) {
		if (i.op_ >= instr::jmp and i.op_ <= instr::jlz) {
			if (std::cmp_greater_equal(i.val, ret.size())) {
				log_debug("Normalized label ", i.val, "/", ret.size(), "->0");
				i.val = 0;
			}
		}
	}
	return ret;
}
