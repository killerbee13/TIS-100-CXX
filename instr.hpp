/* *****************************************************************************
 * TIS-100-CXX
 * Copyright (c) 2025 killerbee, Andrea Stacchiotti
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
#ifndef T21_IMPL_HPP
#define T21_IMPL_HPP

#include "tis100.hpp"
#include "utils.hpp"

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
	// stores either immediate value or jump target
	word_t val{};
	port dst{};

	inline word_t target() const {
		assert(jmp <= op_ && op_ <= jlz);
		return val;
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

#endif // T21_IMPL_HPP
