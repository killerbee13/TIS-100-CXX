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

#include "T21.hpp"
#include "logger.hpp"

#include <kblib/variant.h>

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

std::optional<word_t> T21::read(port p, word_t imm) {
	switch (p) {
	case port::left:
	case port::right:
	case port::down:
	case port::up:
	case port::D5:
	case port::D6:
		return do_read(neighbors[kblib::to_unsigned(kblib::etoi(p))], invert(p));
	case port::nil:
		return word_t{};
	case port::acc:
		return acc;
	case port::any:
		for (auto p_ : {port::left, port::right, port::up, port::down, port::D5,
		                port::D6}) {
			if (auto r = read(p_)) {
				last = p_;
				return r;
			}
		}
		return std::nullopt;
	case port::last:
		return read(last);
	case port::immediate:
		return imm;
	default:
		throw std::invalid_argument{""};
	}
}

bool T21::step() {
	auto log = log_debug();
	log << "step(" << x << ',' << y << ',' << +pc << "): ";
	if (code.empty()) {
		log << "empty";
		return false;
	}
	auto& i = code[kblib::to_unsigned(pc)];
	auto old_state = s;
	log << "instruction type: ";
	bool r = kblib::visit_indexed(
	    std::as_const(i.data),
	    [&](kblib::constant<std::size_t, instr::hcf>, seq_instr) {
		    log << "hcf";
		    log << "\n\ts = " << state_name(s);
		    throw std::runtime_error{"HCF"};
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::nop>, seq_instr) {
		    log << "nop";
		    next();
		    s = activity::run;
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::swp>, seq_instr) {
		    log << "swp (" << acc << "<->" << bak << ')';
		    std::swap(acc, bak);
		    s = activity::run;
		    next();
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::sav>, seq_instr) {
		    log << "sav (" << acc << "->" << bak << ')';
		    bak = acc;
		    s = activity::run;
		    next();
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::neg>, seq_instr) {
		    log << "neg (" << acc << ')';
		    acc = -acc;
		    s = activity::run;
		    next();
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::mov>, mov_instr i) {
		    log << "mov ";
		    if (s == activity::write) {
			    log << "stalled[W]";
			    // if waiting for a write, then this instruction's read already
			    // happened
			    return false;
		    } else if (auto r = read(i.src, i.val)) {
			    log << '(' << *r << ") ";
			    switch (i.dst) {
			    case port::acc:
				    log << "acc = " << *r;
				    acc = *r;
				    [[fallthrough]];
			    case port::nil:
				    // log << "nil = " << *r;
				    s = activity::run;
				    next();
				    break;
			    case port::last:
				    if (last == port::nil) {
					    log << "last[N/A] = " << *r;
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
				    wrt = *r;
				    log << "stalling[W]";
				    // writes don't happen until next cycle
				    break;
			    case port::immediate:
				    assert(i.dst != port::immediate);
			    }
			    return true;
		    } else {
			    s = activity::read;
			    log << "stalled[R]";
			    return s != old_state;
		    }
	    },
	    [&, this](kblib::constant<std::size_t, instr::add>, arith_instr i) {
		    log << "add (" << acc << ") ";
		    if (auto r = read(i.src, i.val)) {
			    log << *r;
			    acc = sat_add(acc, *r);
			    s = activity::run;
			    next();
			    return true;
		    } else {
			    log << "stalled[R]";
			    s = activity::read;
			    return s != old_state;
		    }
	    },
	    [&, this](kblib::constant<std::size_t, instr::sub>, arith_instr i) {
		    log << "sub (" << acc << ") ";
		    if (auto r = read(i.src, i.val)) {
			    log << *r;
			    acc = sat_sub(acc, *r);
			    s = activity::run;
			    next();
			    return true;
		    } else {
			    log << "stalled[R]";
			    s = activity::read;
			    return s != old_state;
		    }
	    },
	    [&, this](kblib::constant<std::size_t, instr::jmp>, jmp_instr i) {
		    log << "jmp " << +i.target;
		    pc = i.target;
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::jez>, jmp_instr i) {
		    log << "jez (" << (acc == 0 ? "taken" : "not taken") << ") "
		        << +i.target;
		    if (acc == 0) {
			    pc = i.target;
		    } else {
			    next();
		    }
		    s = activity::run;
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::jnz>, jmp_instr i) {
		    log << "jnz (" << (acc != 0 ? "taken" : "not taken") << ") "
		        << +i.target;
		    if (acc != 0) {
			    pc = i.target;
		    } else {
			    next();
		    }
		    s = activity::run;
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::jgz>, jmp_instr i) {
		    log << "jgz (" << (acc > 0 ? "taken" : "not taken") << ") "
		        << +i.target;
		    if (acc > 0) {
			    pc = i.target;
		    } else {
			    next();
		    }
		    s = activity::run;
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::jlz>, jmp_instr i) {
		    log << "jlz (" << (acc < 0 ? "taken" : "not taken") << ") "
		        << +i.target;
		    if (acc < 0) {
			    pc = i.target;
		    } else {
			    next();
		    }
		    s = activity::run;
		    return true;
	    },
	    [&, this](kblib::constant<std::size_t, instr::jro>, jro_instr i) {
		    log << "jro ";
		    if (auto r = read(i.src, i.val)) {
			    log << '(' << +pc << '+' << *r << " -> ";
			    pc = sat_add(static_cast<word_t>(pc), *r, word_t{},
			                 static_cast<word_t>(code.size() - 1));
			    log << +pc << ")";
			    s = activity::run;
			    return true;
		    } else {
			    log << "stalled[R]";
			    s = activity::read;
			    return s != old_state;
		    }
	    });
	log << "\n                    s = " << state_name(s);
	return r;
}
bool T21::finalize() {
	if (code.empty()) {
		return false;
	}
	auto& i = code[kblib::to_unsigned(pc)];
	return kblib::visit2(
	    i.data,
	    [this](auto) {
		    log_debug("finalize(", x, ',', y, ',', +pc, "): skipped");
		    return false;
	    },
	    [this](mov_instr i) {
		    auto log = log_debug();
		    log << "finalize(" << x << ',' << y << ',' << +pc << "): mov ";
		    bool r{};
		    if (s == activity::write) {
			    // if write just started
			    if (write_port == port::nil) {
				    log << "started";
				    if (i.dst == port::last) {
					    write_port = last;
				    } else {
					    write_port = i.dst;
				    }
				    r = true;
				    // if write completed
			    } else if (write_port == port::immediate) {
				    log << "completed";
				    write_port = port::nil;
				    s = activity::run;
				    next();
				    r = true;
			    } else {
				    log << "in progress";
			    }
		    } else {
			    log << "skipped";
		    }
		    return r;
	    });
}

std::optional<word_t> T21::emit(port p) {
	assert(p >= port::left and p <= port::D6);
	if (write_port == port::any or write_port == p) {
		auto r = std::exchange(wrt, word_t{});
		if (write_port == port::any) {
			last = p;
		}
		// set write port to flag that the write has completed
		write_port = port::immediate;
		return r;
	} else {
		return std::nullopt;
	}
}

std::string T21::print() const {
	return kblib::concat(
	    '(', x, ',', y, ") T21 { ", pad_right(acc, 4), " (", pad_right(bak, 4),
	    ") ", pad_right(port_name(last), 5), ' ', pad_right(state_name(s), 4),
	    ' ', pad_left(pc, 2), " [",
	    code.empty() ? "" : to_string(code[kblib::to_unsigned(pc)]), "] ", wrt,
	    "->", port_name(write_port), " }");
}
