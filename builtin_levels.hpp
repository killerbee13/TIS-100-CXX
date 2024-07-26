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
#include "random_levels.hpp"

#include <array>
#include <string_view>

struct level_layout {
	std::string_view segment;
	std::string_view name;
	std::string_view layout;
};

inline constexpr std::array<level_layout, 51> layouts{{
    {"00150", "SELF-TEST DIAGNOSTIC",
     "BCBBBCBCBCBB I0 NUMERIC @0 I3 NUMERIC @1 O0 NUMERIC @5 O3 NUMERIC @6"},
    {"10981", "SIGNAL AMPLIFIER", "BBBCBBBBCBBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"20176", "DIFFERENTIAL CONVERTER",
     "BBBBBBBCBBBB I1 NUMERIC @0 I2 NUMERIC @1 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"21340", "SIGNAL COMPARATOR",
     "BBBBBCCCBBBB I0 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6 O3 NUMERIC @7"},
    {"22280", "SIGNAL MULTIPLEXER",
     "BBBBBBBBCBBB I1 NUMERIC @0 I2 NUMERIC @1 I3 NUMERIC @2 O2 NUMERIC @5"},
    {"30647", "SEQUENCE GENERATOR",
     "BBBBBBBBBCBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"31904", "SEQUENCE COUNTER",
     "BBBCBBBBBBBB I1 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"32050", "SIGNAL EDGE DETECTOR",
     "BBBBBBBBCBBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"33762", "INTERRUPT HANDLER",
     "BBBBBBBBCBBB I1 NUMERIC @0 I2 NUMERIC @1 I3 NUMERIC @2 I4 "
     "NUMERIC @3 O2 NUMERIC @5"},
    {"USEG0", "SIMPLE SANDBOX", "BBBBBBBBBBBB I1 NUMERIC - O2 NUMERIC -"},
    {"40196", "SIGNAL PATTERN DETECTOR",
     "BBBCBBBBBBBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"41427", "SEQUENCE PEAK DETECTOR",
     "BBBBBBBCBBBB I1 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"42656", "SEQUENCE REVERSER", "BBSBBBBBCSBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"43786", "SIGNAL MULTIPLIER",
     "BBBBSBBSCBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"USEG1", "STACK MEMORY SANDBOX", "BBSBBBBBBSBB I1 NUMERIC - O2 NUMERIC -"},
    {"50370", "IMAGE TEST PATTERN 1", "BBBBCBBBBBBB O2 IMAGE @5 30 18"},
    {"51781", "IMAGE TEST PATTERN 2", "CBBBBBBBBBBB O2 IMAGE @5 30 18"},
    {"52544", "EXPOSURE MASK VIEWER",
     "BBBCBBBBBBBB I1 NUMERIC @0 O2 IMAGE @5 30 18"},
    {"53897", "HISTOGRAM VIEWER",
     "BBBBBBBBCBBB I1 NUMERIC @0 O2 IMAGE @5 30 18"},
    {"USEG2", "IMAGE CONSOLE SANDBOX",
     "BBBBBBBBBBBB I1 NUMERIC - O2 IMAGE - 36 22"},
    {"60099", "SIGNAL WINDOW FILTER",
     "CBBSBBBBBBBS I1 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"61212", "SIGNAL DIVIDER",
     "BBBBSBBSBBBC I1 NUMERIC @0 I2 NUMERIC @1 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"62711", "SEQUENCE INDEXER",
     "BSBCBBBBBSBB I0 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"63534", "SEQUENCE SORTER", "CBSBBBBBBSBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"70601", "STORED IMAGE DECODER",
     "BBBBBBBBBBBB I1 NUMERIC @0 O2 IMAGE @5 30 18"},
    {"UNKNOWN", "UNKNOWN",
     "BBBCBBBCCBBB I1 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"NEXUS.00.526.6", "SEQUENCE MERGER",
     "BBCBSBBBBBBS I1 NUMERIC @0 I3 NUMERIC @1 O2 NUMERIC @5"},
    {"NEXUS.01.874.8", "INTEGER SERIES CALCULATOR",
     "BBBCBBBBBBBC I1 NUMERIC @0 O1 NUMERIC @5"},
    {"NEXUS.02.981.2", "SEQUENCE RANGE LIMITER",
     "BBBCBBBBBBBB I0 NUMERIC @0 I1 NUMERIC @1 I2 NUMERIC @2 O1 NUMERIC @5"},
    {"NEXUS.03.176.9", "SIGNAL ERROR CORRECTOR",
     "CBBCBBBBBBBB I1 NUMERIC @0 I2 NUMERIC @1 O1 NUMERIC @5 O2 NUMERIC @6"},
    {"NEXUS.04.340.5", "SUBSEQUENCE EXTRACTOR",
     "BBBBBBBBCBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"NEXUS.05.647.1", "SIGNAL PRESCALER",
     "BCCCBBBBBBBB I0 NUMERIC @0 O1 NUMERIC @5 O2 NUMERIC @6 O3 NUMERIC @7"},
    {"NEXUS.06.786.0", "SIGNAL AVERAGER",
     "BBBBCBBBBBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"NEXUS.07.050.0", "SUBMAXIMUM SELECTOR",
     "BBBBBBBBCBBB I0 NUMERIC @0 I1 NUMERIC @1 I2 NUMERIC @2 I3 "
     "NUMERIC @3 O2 NUMERIC @5"},
    {"NEXUS.08.633.9", "DECIMAL DECOMPOSER",
     "BBBCBBBBBBBB I1 NUMERIC @0 O0 NUMERIC @5 O1 NUMERIC @6 O2 NUMERIC @7"},
    {"NEXUS.09.904.9", "SEQUENCE MODE CALCULATOR",
     "SBSCBBBCBBBC I1 NUMERIC @0 O1 NUMERIC @5"},
    {"NEXUS.10.656.5", "SEQUENCE NORMALIZER",
     "BBSBBBBSBCBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"NEXUS.11.711.2", "IMAGE TEST PATTERN 3",
     "CBBBBBBBBBBB O2 IMAGE @5 30 18"},
    {"NEXUS.12.534.4", "IMAGE TEST PATTERN 4",
     "CBBBBBBBBBBB O2 IMAGE @5 30 18"},
    {"NEXUS.13.370.9", "SPATIAL PATH VIEWER",
     "BBBBBBBBCBBB I1 NUMERIC @0 O2 IMAGE @5 30 18"},
    {"NEXUS.14.781.3", "CHARACTER TERMINAL",
     "SBBCBBBBSBBB I1 NUMERIC @0 O2 IMAGE @5 30 18"},
    {"NEXUS.15.897.9", "BACK-REFERENCE REIFIER",
     "BBBBBBBBCBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"NEXUS.16.212.8", "DYNAMIC PATTERN DETECTOR",
     "BBBBBBBBBBBC I0 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"NEXUS.17.135.0", "SEQUENCE GAP INTERPOLATOR",
     "CBBBCSBSCBBB I2 NUMERIC @0 O2 NUMERIC @5"},
    {"NEXUS.18.427.7", "DECIMAL TO OCTAL CONVERTER",
     "BBBBBBBBCBBB I1 NUMERIC @0 O2 NUMERIC @5"},
    {"NEXUS.19.762.9", "PROLONGED SEQUENCE SORTER",
     "CSBBBBBBBSBB I2 NUMERIC @0 O2 NUMERIC @5"},
    {"NEXUS.20.433.1", "PRIME FACTOR CALCULATOR",
     "BBBCBBBBBBBB I1 NUMERIC @0 O1 NUMERIC @5"},
    {"NEXUS.21.601.6", "SIGNAL EXPONENTIATOR",
     "BBBBSBBSCBBB I1 NUMERIC @0 I2 NUMERIC @1 O2 NUMERIC @5"},
    {"NEXUS.22.280.8", "T20 NODE EMULATOR",
     "BBBBBBBBBBBC I1 NUMERIC @0 I2 NUMERIC @1 O1 NUMERIC @5"},
    {"NEXUS.23.727.9", "T31 NODE EMULATOR",
     "CSBBBBBBBSBB I2 NUMERIC @0 O2 NUMERIC @5"},
    {"NEXUS.24.511.7", "WAVE COLLAPSE SUPERVISOR",
     "BBBBBBBBBBBB I0 NUMERIC @0 I1 NUMERIC @1 I2 NUMERIC @2 I3 "
     "NUMERIC @3 O1 NUMERIC @5"},
}};

constexpr int level_id(std::string_view s) {
	for (auto i : kblib::range(layouts.size())) {
		if (s == layouts[i].segment or s == layouts[i].name) {
			return static_cast<int>(i);
		}
	}
	throw std::invalid_argument{
	    kblib::concat("invalid level ID ", kblib::quoted(s))};
}

consteval int operator""_lvl(const char* s, std::size_t size) {
	return level_id(std::string_view(s, size));
}

inline image_t checkerboard(std::ptrdiff_t w, std::ptrdiff_t h) {
	image_t ret(w, h);
	for (const auto y : kblib::range(h)) {
		for (const auto x : kblib::range(w)) {
			ret.at(x, y)
			    = ((x + y % 2) % 2) ? tis_pixel::C_black : tis_pixel::C_white;
		}
	}
	return ret;
}

using tests_t = std::array<inputs_outputs, layouts.size()>;

inline tests_t populate_builtins() {
	auto r = tests_t{{
	    // SELF-TEST DIAGNOSTIC
	    {.data = {{
	         {.inputs = {{51, 62, 16, 83, 61, 14, 35, 17, 63, 48, 22, 40, 29,
	                      50, 77, 32, 31, 49, 89, 89, 12, 59, 53, 75, 37, 78,
	                      57, 38, 44, 98, 85, 25, 80, 39, 20, 16, 91, 81, 84},
	                     {68, 59, 59, 49, 82, 16, 45, 88, 31, 74, 77, 71, 18,
	                      70, 48, 35, 73, 85, 91, 53, 30, 41, 19, 61, 62, 18,
	                      26, 13, 59, 83, 95, 55, 73, 84, 40, 22, 14, 28, 90}},
	          .n_outputs
	          = {{51, 62, 16, 83, 61, 14, 35, 17, 63, 48, 22, 40, 29,
	              50, 77, 32, 31, 49, 89, 89, 12, 59, 53, 75, 37, 78,
	              57, 38, 44, 98, 85, 25, 80, 39, 20, 16, 91, 81, 84},
	             {68, 59, 59, 49, 82, 16, 45, 88, 31, 74, 77, 71, 18,
	              70, 48, 35, 73, 85, 91, 53, 30, 41, 19, 61, 62, 18,
	              26, 13, 59, 83, 95, 55, 73, 84, 40, 22, 14, 28, 90}},
	          .i_output = {}},
	         {.inputs = {{}}, .n_outputs = {{}}, .i_output = {}},
	         {.inputs = {{}}, .n_outputs = {{}}, .i_output = {}},
	     }}},
	    // SIGNAL AMPLIFIER
	    {},
	    // DIFFERENTIAL CONVERTER
	    {.data
	     = {{{.inputs = {{44, 78, 88, 95, 65, 63, 41, 26, 87, 75, 21, 21, 62,
	                      43, 26, 45, 13, 26, 30, 33, 34, 24, 39, 55, 54, 52,
	                      67, 18, 77, 41, 31, 68, 28, 19, 97, 76, 27, 55, 89},
	                     {93, 60, 92, 68, 56, 30, 90, 65, 94, 92, 62, 35, 63,
	                      57, 45, 40, 81, 11, 35, 20, 85, 29, 86, 84, 36, 18,
	                      33, 87, 87, 54, 82, 69, 31, 18, 79, 24, 34, 67, 74}},
	          .n_outputs
	          = {{-49, 18,  -4,  27,  9,   33, -49, -39, -7, -17, -41, -14, -1,
	              -14, -19, 5,   -68, 15,  -5, 13,  -51, -5, -47, -29, 18,  34,
	              34,  -69, -10, -13, -51, -1, -3,  1,   18, 52,  -7,  -12, 15},
	             {49,  -18, 4,  -27, -9,  -33, 49,  39, 7,   17,  41, 14,  1,
	              14,  19,  -5, 68,  -15, 5,   -13, 51, 5,   47,  29, -18, -34,
	              -34, 69,  10, 13,  51,  1,   3,   -1, -18, -52, 7,  12,  -15}},
	          .i_output = {}}}}},
	    // SIGNAL COMPARATOR
	    {},
	    // SIGNAL MULTIPLEXER
	    {.data
	     = {{{.inputs = {{-13, -27, -29, -17, -19, -17, 0,   -28, -17, -28,
	                      -1,  -3,  -3,  -26, -24, -17, -6,  -5,  -6,  -27,
	                      -19, -27, -14, -13, -29, -16, -9,  -24, -2,  -2,
	                      -20, -19, -3,  -15, -50, -55, -22, -46, -27},
	                     {-1, 1,  1, -1, -1, 0, 1, 1,  1, 0, 0, 1, 0,
	                      -1, -1, 0, 1,  -1, 0, 0, 1,  1, 0, 0, 1, -1,
	                      0,  -1, 1, 0,  0,  0, 0, -1, 1, 1, 0, 1, -1},
	                     {7,  7,  6,  17, 11, 29, 25, 29, 3,  7,  11, 14, 27,
	                      5,  21, 26, 8,  25, 25, 15, 25, 3,  17, 20, 6,  11,
	                      18, 4,  4,  17, 4,  24, 11, 29, 19, 27, 12, 12, 11}},
	          .n_outputs
	          = {{-13, 7,   6, -17, -19, 12, 25,  29,  3,  -21, 10,  14, 24,
	              -26, -24, 9, 8,   -5,  19, -12, 25,  3,  3,   7,   6,  -16,
	              9,   -24, 4, 15,  -16, 5,  8,   -15, 19, 27,  -10, 12, -27}},
	          .i_output = {}}}}},
	    // SEQUENCE GENERATOR
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
	    // SEQUENCE COUNTER
	    {},
	    // SIGNAL EDGE DETECTOR
	    {},
	    // INTERRUPT HANDLER
	    {},
	    // SIMPLE SANDBOX
	    {},
	    // SIGNAL PATTERN DETECTOR
	    {},
	    // SEQUENCE PEAK DETECTOR
	    {},
	    // SEQUENCE REVERSER
	    {},
	    // SIGNAL MULTIPLIER
	    {},
	    // STACK MEMORY SANDBOX
	    {},
	    // IMAGE TEST PATTERN 1
	    {.data = {{{.inputs = {},
	                .n_outputs = {},
	                .i_output = {30, 18, tis_pixel::C_white}},
	               {.inputs = {},
	                .n_outputs = {},
	                .i_output = {30, 18, tis_pixel::C_white}},
	               {.inputs = {},
	                .n_outputs = {},
	                .i_output = {30, 18, tis_pixel::C_white}}}}},
	    // IMAGE TEST PATTERN 2
	    {.data
	     = {{{.inputs = {}, .n_outputs = {}, .i_output = checkerboard(30, 18)},
	         {.inputs = {}, .n_outputs = {}, .i_output = checkerboard(30, 18)},
	         {.inputs = {},
	          .n_outputs = {},
	          .i_output = checkerboard(30, 18)}}}},
	    // EXPOSURE MASK VIEWER
	    {},
	    // HISTOGRAM VIEWER
	    {},
	    // IMAGE CONSOLE SANDBOX
	    {},
	    // SIGNAL WINDOW FILTER
	    {},
	    // SIGNAL DIVIDER
	    {},
	    // SEQUENCE INDEXER
	    {},
	    // SEQUENCE SORTER
	    {},
	    // STORED IMAGE DECODER
	    {},
	    // SEQUENCE MERGER
	    {},
	    // INTEGER SERIES CALCULATOR
	    {},
	    // SEQUENCE RANGE LIMITER
	    {},
	    // SIGNAL ERROR CORRECTOR
	    {},
	    // SUBSEQUENCE EXTRACTOR
	    {},
	    // SIGNAL PRESCALER
	    {},
	    // SIGNAL AVERAGER
	    {},
	    // SUBMAXIMUM SELECTOR
	    {},
	    // DECIMAL DECOMPOSER
	    {},
	    // SEQUENCE MODE CALCULATOR
	    {},
	    // SEQUENCE NORMALIZER
	    {},
	    // IMAGE TEST PATTERN 3
	    {},
	    // IMAGE TEST PATTERN 4
	    {},
	    // SPATIAL PATH VIEWER
	    {},
	    // CHARACTER TERMINAL
	    {},
	    // BACK-REFERENCE REIFIER
	    {},
	    // DYNAMIC PATTERN DETECTOR
	    {},
	    // SEQUENCE GAP INTERPOLATOR
	    {},
	    // DECIMAL TO OCTAL CONVERTER
	    {},
	    // PROLONGED SEQUENCE SORTER
	    {},
	    // PRIME FACTOR CALCULATOR
	    {},
	    // SIGNAL EXPONENTIATOR
	    {},
	    // T20 NODE EMULATOR
	    {},
	    // T31 NODE EMULATOR
	    {},
	    // WAVE COLLAPSE SUPERVISOR
	    {},
	    // UNKNOWN
	    {},
	}};
	return r;
}

inline const tests_t tests = populate_builtins();

inline bool check_achievement(int id, const field& solve, score sc) {
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
					        if (auto* m = std::get_if<instr::mov>(&i.data);
					            m and (m->dst <= port::D6)) {
						        if (type(p->neighbors[static_cast<std::size_t>(
						                m->dst)])
						            == node::T30) {
							        return true;
						        }
					        } else if (m and m->dst == port::any) {
						        for (auto n : p->neighbors) {
							        if (type(n) == node::T30) {
								        return true;
							        }
						        }
					        }
					        return false;
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
