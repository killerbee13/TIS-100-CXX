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
#ifndef T21_HPP
#define T21_HPP

#include "node.hpp"

#include <string>
#include <variant>
#include <vector>

struct seq_instr {};
struct mov_instr {
	port src{}, dst{};
	word_t val;
};
struct arith_instr {
	port src{};
	word_t val;
};
struct jmp_instr {
	index_t target;
};
struct jro_instr {
	port src{};
	index_t val;
};

struct T21;

struct instr {
	enum op {
		nop,
		swp,
		sav,
		neg,
		hcf, // 4
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
	// variant index corresponds to instruction op
	std::variant<seq_instr, seq_instr, seq_instr, seq_instr, seq_instr, //
	             mov_instr,                                             //
	             arith_instr, arith_instr,                              //
	             jmp_instr, jmp_instr, jmp_instr, jmp_instr, jmp_instr, //
	             jro_instr>
	    data;

	op get_op() const { return static_cast<op>(data.index()); }
};

std::string to_string(instr i);

struct T21 : node {
	type_t type() const noexcept override { return type_t::T21; }
	void step() override {}
	void read() override {}
	activity state() const noexcept override { return s; }

	word_t acc{}, bak{}, write{};
	index_t pc{};
	port write_port{}, last;
	std::vector<instr> code;
	activity s{};
};

#endif // T21_HPP
