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
#ifndef T21_HPP
#define T21_HPP

#include "node.hpp"
#include "utils.hpp"

#include <span>
#include <string>

struct T21;

struct instr {
	// HCF as opcode 0 makes crashes more likely on OOB code reads
	enum op : std::int8_t {
		hcf,
		nop,
		swp,
		sav,
		neg, // 5
		mov, // 1
		add,
		sub, // 2
		jmp,
		jez,
		jnz,
		jgz,
		jlz, // 5
		jro, // 1
	};
	op op_;
	port src{immediate};
	word_t val{};
	// stores either MOV dst or jump target
	word_t data{};

	inline port dst() const {
		assert(op_ == mov);
		return static_cast<port>(data);
	}
};
constexpr std::string to_string(instr::op o) {
	switch (o) {
	case instr::hcf:
		return "HCF";
	case instr::nop:
		return "NOP";
	case instr::swp:
		return "SWP";
	case instr::sav:
		return "SAV";
	case instr::neg:
		return "NEG";
	case instr::mov:
		return "MOV";
	case instr::add:
		return "ADD";
	case instr::sub:
		return "SUB";
	case instr::jmp:
		return "JMP";
	case instr::jez:
		return "JEZ";
	case instr::jnz:
		return "JNZ";
	case instr::jgz:
		return "JGZ";
	case instr::jlz:
		return "JLZ";
	case instr::jro:
		return "JRO";
	default:
		throw std::invalid_argument{concat("Unknown instr::op ", etoi(o))};
	}
}

std::string to_string(instr i);

struct T21 final : regular_node {
	using regular_node::regular_node;

	type_t type() const noexcept override { return type_t::T21; }

	[[gnu::always_inline]] inline void step(logger& debug) override {
		assert(not code.empty());
		debug << "step(" << x << ',' << y << ',' << pc << "): instruction type: ";
		if (s == activity::write) {
			// if waiting for a write, then this instruction's read already
			// happened
			debug << "MOV stalled[W]" << '\n';
			return;
		}
		auto& instr = code[to_unsigned(pc)];
		debug.log_r([&] { return to_string(instr.op_); });
		auto r = read(instr.src, instr.val);
		if (r == word_empty) {
			debug << " stalled[R]" << '\n';
			s = activity::read;
			return;
		}

		switch (instr.op_) {
		[[unlikely]] case instr::hcf: {
			debug << "\n\ts = " << state_name(s);
			throw hcf_exception{x, y, pc};
		}
		case instr::nop: {
			next();
			s = activity::run;
		} break;
		case instr::swp: {
			debug << " (" << acc << "<->" << bak << ')';
			std::swap(acc, bak);
			s = activity::run;
			next();
		} break;
		case instr::sav: {
			debug << " (" << acc << "->" << bak << ')';
			bak = acc;
			s = activity::run;
			next();
		} break;
		case instr::neg: {
			debug << " (" << acc << ')';
			acc = -acc;
			s = activity::run;
			next();
		} break;
		[[likely]] case instr::mov: {
			debug << " (" << r << ") ";
			switch (instr.dst()) {
			case port::acc:
				debug << "acc = " << r;
				acc = r;
				[[fallthrough]];
			case port::nil:
				// log << "nil = " << *r;
				s = activity::run;
				next();
				break;
			case port::last:
				if (last == port::nil) {
					debug << "last[N/A] = " << r;
					s = activity::run;
					next();
					break;
				}
				[[fallthrough]];
			case port::left:
			case port::right:
			case port::up:
			case port::down:
			case port::D5:
			case port::D6:
			case port::any:
				s = activity::write;
				wrt = r;
				debug << "stalling[W]";
				// writes don't happen until next cycle
				break;
			case port::immediate:
				std::unreachable();
			}
		} break;
		case instr::add: {
			debug << " (" << acc << ") ";
			debug << r;
			acc = sat_add(acc, r);
			s = activity::run;
			next();
		} break;
		case instr::sub: {
			debug << " (" << acc << ") ";
			debug << r;
			acc = sat_sub(acc, r);
			s = activity::run;
			next();
		} break;
		case instr::jmp: {
			debug << " " << +instr.data;
			pc = instr.data;
		} break;
		case instr::jez: {
			debug << " (" << (acc == 0 ? "taken" : "not taken") << ") "
			      << instr.data;
			if (acc == 0) {
				pc = instr.data;
			} else {
				next();
			}
			s = activity::run;
		} break;
		case instr::jnz: {
			debug << " (" << (acc != 0 ? "taken" : "not taken") << ") "
			      << instr.data;
			if (acc != 0) {
				pc = instr.data;
			} else {
				next();
			}
			s = activity::run;
		} break;
		case instr::jgz: {
			debug << " (" << (acc > 0 ? "taken" : "not taken") << ") "
			      << instr.data;
			if (acc > 0) {
				pc = instr.data;
			} else {
				next();
			}
			s = activity::run;
		} break;
		case instr::jlz: {
			debug << " (" << (acc < 0 ? "taken" : "not taken") << ") "
			      << instr.data;
			if (acc < 0) {
				pc = instr.data;
			} else {
				next();
			}
			s = activity::run;
		} break;
		case instr::jro: {
			debug << " ";
			debug << '(' << pc << '+' << r << " -> ";
			pc = sat_add(pc, r, word_t{}, static_cast<word_t>(code.size() - 1));
			debug << pc << ")";
			s = activity::run;
		} break;
		default:
			std::unreachable();
		}
		debug << '\n';
	}

	[[gnu::always_inline]] inline void finalize(logger& debug) override {
		if (s == activity::write) {
			debug << "finalize(" << x << ',' << y << ',' << pc << "): mov ";
			// if write just started
			if (write_port == port::nil) {
				debug << "started";
				port p = code[to_unsigned(pc)].dst();
				if (p == port::last) {
					write_port = last;
				} else {
					write_port = p;
				}
				// if write completed
			} else if (write_port == port::immediate) {
				debug << "completed";
				write_port = port::nil;
				s = activity::run;
				next();
			} else {
				debug << "in progress";
			}
		} else {
			debug << "finalize(" << x << ',' << y << ',' << pc << "): skipped";
		}
		debug << '\n';
	}

	std::unique_ptr<regular_node> clone() const override {
		auto ret = std::make_unique<T21>(x, y);
		ret->set_code(code);
		return ret;
	}

	optional_word emit(port p) override {
		assert(p >= port::dir_first and p <= port::dir_last);
		if (write_port == port::any or write_port == p) {
			auto r = std::exchange(wrt, word_empty);
			if (write_port == port::any) {
				last = p;
			}
			// set write port to flag that the write has completed
			write_port = port::immediate;
			return r;
		} else {
			return word_empty;
		}
	}

	std::string state() const override {
		return concat('(', x, ',', y, ") T21 { ", pad_right(acc, 4), " (",
		              pad_right(bak, 4), ") ", pad_right(port_name(last), 5), ' ',
		              pad_right(state_name(s), 4), ' ', pad_left(pc, 2), " [",
		              code.empty() ? "" : to_string(code[to_unsigned(pc)]), "]",
		              wrt == word_empty
		                  ? ""
		                  : concat(" ", wrt, "->", port_name(write_port)),
		              " }");
	}

	void reset() noexcept {
		acc = 0;
		bak = 0;
		wrt = word_empty;
		pc = 0;
		write_port = port::nil;
		last = port::nil;
		s = activity::idle;
	}
	void set_code(std::span<const instr> new_code) {
		if (new_code.size() <= small_.size()) {
			code = {small_.begin(),
			        std::ranges::copy(new_code, small_.begin()).out};
		} else {
			large_.reset(new instr[new_code.size()]);
			code = {large_.get(), std::ranges::copy(new_code, large_.get()).out};
		}
	}

	bool has_instr(std::same_as<instr::op> auto... ops) const {
		return std::any_of(code.begin(), code.end(), [=](const instr& i) {
			return ((i.op_ == ops) or ...);
		});
	}

	std::span<instr> code;

 private:
	std::unique_ptr<instr[]> large_;
	std::array<instr, def_T21_size> small_;
	word_t acc{}, bak{};
	optional_word wrt = word_empty;
	word_t pc{};
	port write_port{port::nil}, last{port::nil};
	activity s{activity::idle};

	/// Increment the program counter, wrapping to beginning.
	inline void next() { pc = static_cast<word_t>((pc + 1) % code.size()); }
	/// Attempt to read a value from this node's port p, which may be
	/// any, last, or immediate, unlike the general do_read and emit
	/// functions
	[[gnu::always_inline]] inline optional_word read(port p, word_t imm) {
		switch (p) {
		[[likely]] case port::immediate:
			return imm;
		case port::left:
		case port::right:
		case port::down:
		case port::up:
		case port::D5:
		case port::D6:
			return do_read(p);
		case port::nil:
			return word_t{};
		case port::acc:
			return acc;
		case port::any:
			for (auto p_ = port::dir_first; p_ <= port::dir_last; p_++) {
				if (auto r = do_read(p_); r != word_empty) {
					last = p_;
					return r;
				}
			}
			return word_empty;
		case port::last:
			return last == port::nil ? word_t{} : do_read(last);
		default:
			std::unreachable();
		}
	}
};

#endif // T21_HPP
