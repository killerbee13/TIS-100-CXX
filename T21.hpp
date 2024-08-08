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

#include <span>
#include <string>
#include <vector>

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

struct T21 : node {
	using node::node;
	type_t type() const noexcept override { return type_t::T21; }
	void step() override;
	void finalize() override;
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
	std::string state() const override;

	word_t acc{}, bak{}, wrt{};
	word_t pc{};
	port write_port{port::nil}, last{port::nil};
	std::span<instr> code;
	activity s{activity::idle};

	std::unique_ptr<instr[]> large_;
	std::array<instr, def_T21_size> small_;

 private:
	/// Increment the program counter, wrapping to beginning.
	void next();
	/// Attempt to read a value from this node's port p, which may be
	/// any, last, or immediate, unlike the general do_read and read_
	/// functions
	std::optional<word_t> read(port p, word_t imm = 0);
};

#endif // T21_HPP
