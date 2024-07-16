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
#ifndef BUILTIN_LEVELS_HPP
#define BUILTIN_LEVELS_HPP

#include "parser.hpp"

#include <array>
#include <kblib/simple.h>
#include <string_view>

inline constexpr std::array<std::array<std::string_view, 2>, 51> layouts{{
    {"00150",
     "BCBBBCBCBCBB I0 NUMERIC @0 I3 NUMERIC @1 O0 NUMERIC @5 O3 NUMERIC @6"},
    {"10981", "BBBCBBBBCBBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"20176",
     "BBBBBBBCBBBB I1 NUMERIC @0 I2 NUMERIC @1 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"21340",
     "BBBBBCCCBBBB I0 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6 O3 NUMERIC @7"},
    {"22280",
     "BBBBBBBBCBBB I1 NUMERIC @0 I2 NUMERIC @1 I3 NUMERIC @2 O2 NUMERIC @5"},
    {"30647", "BBBBBBBBBCBB I1 NUMERIC @0 I2 NUMERIC @1 O3 NUMERIC @5"},
    {"31904", "BBBCBBBBBBBB I1 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"32050", "BBBBBBBBCBBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"33762", "BBBBBBBBCBBB I1 NUMERIC @0 I2 NUMERIC @1 I3 NUMERIC @2 I4 "
              "NUMERIC @3 O2 NUMERIC @5"},
    {"USEG0", "BBBBBBBBBBBB I1 NUMERIC - O2 NUMERIC -"},
    {"40196", "BBBCBBBBBBBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"41427", "BBBBBBBCBBBB I1 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"42656", "BBSBBBBBCSBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"43786", "BBBBSBBSCBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"USEG1", "BBSBBBBBBSBB I1 NUMERIC - O2 NUMERIC -"},
    {"50370", "BBBBCBBBBBBB O2 IMAGE @5"},
    {"51781", "CBBBBBBBBBBB O2 IMAGE @5"},
    {"52544", "BBBCBBBBBBBB I1 NUMERIC @0 O2 IMAGE @5"},
    {"53897", "BBBBBBBBCBBB I1 NUMERIC @0 O2 IMAGE @5"},
    {"USEG2", "BBBBBBBBBBBB I1 NUMERIC - O2 IMAGE -"},
    {"60099", "CBBSBBBBBBBS I1 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"61212",
     "BBBBSBBSBBBC I1 NUMERIC @0 I2 NUMERIC @1 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"62711", "BSBCBBBBBSBB I0 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"63534", "CBSBBBBBBSBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"70601", "BBBBBBBBBBBB I1 NUMERIC @0 O2 IMAGE @5"},
    {"00.526.6", "BBCBSBBBBBBS I1 NUMERIC @0 I3 NUMERIC @1 O2 NUMERIC @5"},
    {"01.874.8", "BBBCBBBBBBBC I1 NUMERIC @0 O1 NUMERIC @5"},
    {"02.981.2",
     "BBBCBBBBBBBB I0 NUMERIC @0 I1 NUMERIC @1; I2 NUMERIC @2 O1 NUMERIC @5"},
    {"03.176.9",
     "CBBCBBBBBBBB I1 NUMERIC @0 I2 NUMERIC @1 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"04.340.5", "BBBBBBBBCBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"05.647.1",
     "BCCCBBBBBBBB I0 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6 O3 NUMERIC @7"},
    {"06.786.0", "BBBBCBBBBBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"07.050.0", "BBBBBBBBCBBB I0 NUMERIC @0 I1 NUMERIC @1 I2 NUMERIC @2 I3 "
                 "NUMERIC @3 O2 NUMERIC @5"},
    {"08.633.9",
     "BBBCBBBBBBBB I1 NUMERIC @0 O0 NUMERIC @5 O1 NUMERIC @6 O2 NUMERIC @7"},
    {"09.904.9", "SBSCBBBCBBBC I1 NUMERIC @0 O1 NUMERIC @5"},
    {"10.656.5", "BBSBBBBSBCBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"11.711.2", "CBBBBBBBBBBB O2 NUMERIC @5"},
    {"12.534.4", "CBBBBBBBBBBB O2 NUMERIC @5"},
    {"13.370.9", "BBBBBBBBCBBB I1 NUMERIC @0 O2 IMAGE @5"},
    {"14.781.3", "SBBCBBBBSBBB I1 NUMERIC @0 O2 IMAGE @5"},
    {"15.897.9", "BBBBBBBBCBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"16.212.8", "BBBBBBBBBBBC I0 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"17.135.0", "CBBBCSBSCBBB I2 NUMERIC @0 O2 NUMERIC @5"},
    {"18.427.7", "BBBBBBBBCBBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"19.762.9", "CSBBBBBBBSBB I2 NUMERIC @0 O2 NUMERIC @5"},
    {"20.433.1", "BBBCBBBBBBBB I1 NUMERIC @0 O1 NUMERIC @5"},
    {"21.601.6", "BBBBSBBSCBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"22.280.8", "BBBBBBBBBBBC I1 NUMERIC @0 I2 NUMERIC @1 O1 NUMERIC @5"},
    {"23.727.9", "CSBBBBBBBSBB I2 NUMERIC @0 O2 NUMERIC @5"},
    {"24.511.7", "BBBBBBBBBBBB I0 NUMERIC @0 I1 NUMERIC @1 I2 NUMERIC @2 I3 "
                 "NUMERIC @3 O1 NUMERIC @5"},
    {"UNKNOWN", "BBBCBBBCCBBB I1 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6"},
}};

constexpr int level_id(std::string_view s) {
	for (auto i : kblib::range(layouts.size())) {
		if (s == layouts[i][0]) {
			return static_cast<int>(i);
		}
	}
	throw std::invalid_argument{""};
}

consteval int operator""_lvl(const char* s, std::size_t size) {
	return level_id(std::string_view(s, size));
}

std::array<inputs_outputs, layouts.size()> tests{{
    {.data = {{
         {.inputs = {{51, 62, 16, 83, 61, 14, 35, 17, 63, 48, 22, 40, 29,
                      50, 77, 32, 31, 49, 89, 89, 12, 59, 53, 75, 37, 78,
                      57, 38, 44, 98, 85, 25, 80, 39, 20, 16, 91, 81, 84},
                     {68, 59, 59, 49, 82, 16, 45, 88, 31, 74, 77, 71, 18,
                      70, 48, 35, 73, 85, 91, 53, 30, 41, 19, 61, 62, 18,
                      26, 13, 59, 83, 95, 55, 73, 84, 40, 22, 14, 28, 90}},
          .n_outputs = {{51, 62, 16, 83, 61, 14, 35, 17, 63, 48, 22, 40, 29,
                         50, 77, 32, 31, 49, 89, 89, 12, 59, 53, 75, 37, 78,
                         57, 38, 44, 98, 85, 25, 80, 39, 20, 16, 91, 81, 84},
                        {68, 59, 59, 49, 82, 16, 45, 88, 31, 74, 77, 71, 18,
                         70, 48, 35, 73, 85, 91, 53, 30, 41, 19, 61, 62, 18,
                         26, 13, 59, 83, 95, 55, 73, 84, 40, 22, 14, 28, 90}},
          .i_output = {}},
         {.inputs = {{}}, .n_outputs = {{}}, .i_output = {}},
         {.inputs = {{}}, .n_outputs = {{}}, .i_output = {}},
     }}},
    {},
    {},
    {},
    {},
    {.data
     = {{{.inputs = {{46, 71, 66, 21, 79, 23, 62, 23, 36, 96, 12, 97, 47},
                     {71, 29, 90, 67, 79, 84, 78, 27, 60, 45, 67, 42, 64}},
          .n_outputs
          = {{46, 71, 0, 29, 71, 0, 66, 90, 0, 21, 67, 0, //
              79, 79, 0, 23, 84, 0, 62, 78, 0, 23, 27, 0, //
              36, 60, 0, 45, 96, 0, 12, 67, 0, 42, 97, 0, 47, 64, 0}},
          .i_output = {}},
         {.inputs = {{71, 29, 90, 67, 98, 84, 78, 27, 60, 45, 67, 37, 64},
                     {39, 72, 55, 40, 83, 22, 53, 19, 15, 67, 66, 37, 82}},
          .n_outputs
          = {{39, 71, 0, 29, 72, 0, 55, 90, 0, 40, 67, 0, //
              83, 98, 0, 22, 84, 0, 53, 78, 0, 19, 27, 0, //
              15, 60, 0, 45, 67, 0, 66, 67, 0, 37, 37, 0, 64, 82, 0}},
          .i_output = {}},
         {.inputs = {{39, 72, 55, 40, 83, 15, 53, 19, 15, 67, 66, 43, 82},
                     {34, 34, 94, 13, 86, 15, 47, 77, 21, 15, 83, 69, 33}},
          .n_outputs
          = {{34, 39, 0, 34, 72, 0, 55, 94, 0, 13, 40, 0, //
              83, 86, 0, 15, 15, 0, 47, 53, 0, 19, 77, 0, //
              15, 21, 0, 15, 67, 0, 66, 83, 0, 43, 69, 0, 33, 82, 0}},
          .i_output = {}}}}},
}};

bool check_achievement(int id, const field& solve, score sc) {
	// SELF-TEST DIAGNOSTIC
	if (id == "00150"_lvl) {
		// BUSY_LOOP
		return sc.cycles > 100000;
		// SIGNAL COMPARATOR
	} else if (id == "21340"_lvl) {
		// UNCONDITIONAL
		for (std::size_t i = 0; i < solve.nodes_used(); ++i) {
			if (auto p = solve.node_by_index(i)) {
				if (std::any_of(p->code.begin(), p->code.end(), [](const instr& i) {
					    auto op = i.get_op();
					    return op == instr::jez or op == instr::jnz
					           or op == instr::jgz or op == instr::jlz;
				    })) {
					return false;
				}
			}
		}
		return true;
		// SEQUENCE REVERSER
	} else if (id == "42656"_lvl) {
		// NO_MEMORY

		// technically this check is wrong, because if you have an instruction
		// that never runs that would write to a T30, I *think* you should get the
		// achievement according to the game's description, but nobody pursuing a
		// record would ever do that so this just checks that no T30 is the
		// destination of a mov
		for (std::size_t i = 0; i < solve.nodes_used(); ++i) {
			if (auto p = solve.node_by_index(i)) {
				if (std::any_of(
				        p->code.begin(), p->code.end(), [&](const instr& i) {
					        if (auto* m = std::get_if<instr::mov>(&i.data)) {
						        return type(p->neighbors[static_cast<std::size_t>(
						                   m->dst)])
						               == node::T30;
					        } else {
						        return false;
					        }
				        })) {
					return false;
				}
			}
		}
		return true;
	} else {
		return false;
	}
}

#endif // BUILTIN_LEVELS_HPP
