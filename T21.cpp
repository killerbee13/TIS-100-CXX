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

#include "T21.hpp"
#include "logger.hpp"
#include "utils.hpp"

using namespace std::literals;

static_assert(sat_add(word_max, word_max) == word_max);
static_assert(sat_add(word_min, word_max) == 0);
static_assert(sat_add(word_min, word_min) == word_min);

constexpr auto wrap_add(auto a, auto b, auto l, auto h) {
	return static_cast<std::common_type_t<decltype(l), decltype(h)>>(
	    (a + b - l) % (h - l) + l);
}

static_assert(15 % 15 == 0);
static_assert(wrap_add(15, 1, 0, 16) == 0);

void T21::next() {
	pc = wrap_add(pc, word_t{1}, word_t{0}, word_t(code.size()));
	return;
}

optional_word T21::read(port p, word_t imm) {
	switch (p) {
	[[likely]] case port::immediate:
		return imm;
	case port::left:
	case port::right:
	case port::down:
	case port::up:
	case port::D5:
	case port::D6:
		return do_read(neighbors[to_unsigned(etoi(p))], invert(p));
	case port::nil:
		return word_t{};
	case port::acc:
		return acc;
	case port::any:
		for (auto p_ = port::left; p_ <= port::D6; p_++) {
			if (auto r = read(p_); r != word_empty) {
				last = p_;
				return r;
			}
		}
		return word_empty;
	case port::last:
		return read(last);
	default:
		std::unreachable();
	}
}

void T21::step(logger& debug) {
	debug << "step(" << x << ',' << y << ',' << +pc << "): ";
	if (code.empty()) {
		debug << "empty" << '\n';
		return;
	}
	auto& instr = code[to_unsigned(pc)];
	debug << "instruction type: ";
	debug.log_r([&] { return to_string(instr.op_); });
	if (s == activity::write) {
		// if waiting for a write, then this instruction's read already
		// happened
		debug << " stalled[W]" << '\n';
		return;
	}
	auto r = read(instr.src, instr.val);
	if (r == word_empty) {
		debug << " stalled[R]" << '\n';
		s = activity::read;
		return;
	}

	switch (instr.op_) {
	case instr::hcf: {
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
		debug << " (" << (acc == 0 ? "taken" : "not taken") << ") " << instr.data;
		if (acc == 0) {
			pc = instr.data;
		} else {
			next();
		}
		s = activity::run;
	} break;
	case instr::jnz: {
		debug << " (" << (acc != 0 ? "taken" : "not taken") << ") " << instr.data;
		if (acc != 0) {
			pc = instr.data;
		} else {
			next();
		}
		s = activity::run;
	} break;
	case instr::jgz: {
		debug << " (" << (acc > 0 ? "taken" : "not taken") << ") " << instr.data;
		if (acc > 0) {
			pc = instr.data;
		} else {
			next();
		}
		s = activity::run;
	} break;
	case instr::jlz: {
		debug << " (" << (acc < 0 ? "taken" : "not taken") << ") " << instr.data;
		if (acc < 0) {
			pc = instr.data;
		} else {
			next();
		}
		s = activity::run;
	} break;
	case instr::jro: {
		debug << " ";
		debug << '(' << +pc << '+' << r << " -> ";
		pc = sat_add(pc, r, word_t{}, static_cast<word_t>(code.size() - 1));
		debug << +pc << ")";
		s = activity::run;
	} break;
	default:
		std::unreachable();
	}
	debug << '\n';
}

void T21::finalize(logger& debug) {
	if (code.empty()) {
		return;
	}
	if (s == activity::write) {
		debug << "finalize(" << x << ',' << y << ',' << +pc << "): mov ";
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
		debug << "finalize(" << x << ',' << y << ',' << +pc << "): skipped";
	}
	debug << '\n';
}

std::unique_ptr<node> T21::clone() const {
	auto ret = std::make_unique<T21>(x, y);
	ret->set_code(code);
	return ret;
}

optional_word T21::emit(port p) {
	assert(p >= port::left and p <= port::D6);
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

std::string T21::state() const {
	return concat(
	    '(', x, ',', y, ") T21 { ", pad_right(acc, 4), " (", pad_right(bak, 4),
	    ") ", pad_right(port_name(last), 5), ' ', pad_right(state_name(s), 4),
	    ' ', pad_left(pc, 2), " [",
	    code.empty() ? "" : to_string(code[to_unsigned(pc)]), "]",
	    wrt == word_empty ? "" : concat(" ", wrt, "->", port_name(write_port)),
	    " }");
}
