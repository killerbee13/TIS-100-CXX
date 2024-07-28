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
// Note that label names are not stored
struct jmp_instr {
	index_t target;
};
struct jro_instr {
	port src{};
	index_t val;
};

struct T21;

struct instr {
	// HCF as opcode 0 makes crashes more likely on OOB code reads
	enum op {
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
	// variant index corresponds to instruction op
	std::variant<seq_instr, seq_instr, seq_instr, seq_instr, seq_instr, //
	             mov_instr,                                             //
	             arith_instr, arith_instr,                              //
	             jmp_instr, jmp_instr, jmp_instr, jmp_instr, jmp_instr, //
	             jro_instr>
	    data;

	op get_op() const { return static_cast<op>(data.index()); }
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
		throw std::invalid_argument{
		    kblib::concat("Unknown instr::op ", kblib::etoi(o))};
	}
}

std::string to_string(instr i);

struct T21 : node {
	using node::node;
	type_t type() const noexcept override { return type_t::T21; }
	bool step() override;
	bool finalize() override;
	void reset() noexcept override {
		acc = 0;
		bak = 0;
		wrt = 0;
		pc = 0;
		write_port = port::nil;
		last = port::nil;
		s = activity::idle;
	}
	std::optional<word_t> emit(port) override;
	activity state() const noexcept override { return s; }
	std::string print() const override;

	word_t acc{}, bak{}, wrt{};
	index_t pc{};
	port write_port{port::nil}, last{port::nil};
	std::vector<instr> code;
	activity s{activity::idle};

 private:
	/// Increment the program counter, wrapping to beginning.
	void next();
	/// Attempt to read a value from this node's port p, which may be
	/// any, last, or immediate, unlike the general do_read and read_
	/// functions
	std::optional<word_t> read(port p, word_t imm = 0);
};

#endif // T21_HPP
