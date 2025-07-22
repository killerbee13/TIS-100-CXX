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
#ifndef BUILTIN_SPECS_HPP
#define BUILTIN_SPECS_HPP

#include "tis100.hpp"

#include <array>
#include <vector>

inline constexpr size_t builtin_levels_num = 51;

// use placeholder value 1 for sandbox levels
inline constexpr std::array<uint32_t, builtin_levels_num> builtin_seeds{
    50,      2,       3,       4,       22,      //
    5,       9,       7,       19,      1,       //
    888,     18,      10,      6,       1,       //
    13,      14,      60,      15,      1,       //
    55,      16,      11,      12,      21,      //
    23,                                          //
    0 * 23,  1 * 23,  2 * 23,  3 * 23,  4 * 23,  //
    5 * 23,  6 * 23,  7 * 23,  8 * 23,  9 * 23,  //
    10 * 23, 11 * 23, 12 * 23, 13 * 23, 14 * 23, //
    15 * 23, 16 * 23, 17 * 23, 18 * 23, 19 * 23, //
    20 * 23, 21 * 23, 22 * 23, 23 * 23, 24 * 23, //
};

struct standard_layout_spec {
	std::array<std::array<node_type_t, 4>, 3> nodes;
	std::array<node_type_t, 4> inputs;
	std::array<node_type_t, 4> outputs;
};
struct dynamic_layout_spec {
	std::vector<std::vector<node_type_t>> nodes;
	std::vector<node_type_t> inputs;
	std::vector<node_type_t> outputs;
};

struct builtin_level_layout {
	std::string_view segment;
	std::string_view name;
	standard_layout_spec layout;
};

// clang-format off

// the reason this is a lambda is the "using enum" line at the beginning
// this needs to sit on an header file where it doesn't see the node type
// classes, or there will be a name conflict
inline constexpr std::array builtin_layouts = []{
	using enum node_type_t;
	return std::array<builtin_level_layout, builtin_levels_num>{{
	{"00150", "SELF-TEST DIAGNOSTIC", {
		.nodes = {{
			{T21, Damaged, T21, T21, },
			{T21, Damaged, T21, Damaged, },
			{T21, Damaged, T21, T21, },
		}},
		.inputs = {{in, null, null, in, }},
		.outputs = {{out, null, null, out, }},
	}},
	{"10981", "SIGNAL AMPLIFIER", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"20176", "DIFFERENTIAL CONVERTER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, out, out, null, }},
	}},
	{"21340", "SIGNAL COMPARATOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, Damaged, Damaged, Damaged, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{in, null, null, null, }},
		.outputs = {{null, out, out, out, }},
	}},
	{"22280", "SIGNAL MULTIPLEXER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, in, in, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"30647", "SEQUENCE GENERATOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, Damaged, T21, T21, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"31904", "SEQUENCE COUNTER", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, out, out, null, }},
	}},
	{"32050", "SIGNAL EDGE DETECTOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"33762", "INTERRUPT HANDLER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{in, in, in, in, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"USEG0", "SIMPLE SANDBOX", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"40196", "SIGNAL PATTERN DETECTOR", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"41427", "SEQUENCE PEAK DETECTOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, out, out, null, }},
	}},
	{"42656", "SEQUENCE REVERSER", {
		.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T30, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"43786", "SIGNAL MULTIPLIER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"USEG1", "STACK MEMORY SANDBOX", {
		.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"50370", "IMAGE TEST PATTERN 1", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, null, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"51781", "IMAGE TEST PATTERN 2", {
		.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, null, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"52544", "EXPOSURE MASK VIEWER", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"53897", "HISTOGRAM VIEWER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"USEG2", "IMAGE CONSOLE SANDBOX", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"60099", "SIGNAL WINDOW FILTER", {
		.nodes = {{
			{Damaged, T21, T21, T30, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T30, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, out, out, null, }},
	}},
	{"61212", "SIGNAL DIVIDER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, out, out, null, }},
	}},
	{"62711", "SEQUENCE INDEXER", {
		.nodes = {{
			{T21, T30, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs = {{in, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"63534", "SEQUENCE SORTER", {
		.nodes = {{
			{Damaged, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"70601", "STORED IMAGE DECODER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"UNKNOWN", "UNKNOWN", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, Damaged, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, out, out, null, }},
	}},
	{"NEXUS.00.526.6", "SEQUENCE MERGER", {
		.nodes = {{
			{T21, T21, Damaged, T21, },
			{T30, T21, T21, T21, },
			{T21, T21, T21, T30, },
		}},
		.inputs = {{null, in, null, in, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.01.874.8", "INTEGER SERIES CALCULATOR", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, out, null, null, }},
	}},
	{"NEXUS.02.981.2", "SEQUENCE RANGE LIMITER", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{in, in, in, null, }},
		.outputs = {{null, out, null, null, }},
	}},
	{"NEXUS.03.176.9", "SIGNAL ERROR CORRECTOR", {
		.nodes = {{
			{Damaged, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, out, out, null, }},
	}},
	{"NEXUS.04.340.5", "SUBSEQUENCE EXTRACTOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.05.647.1", "SIGNAL PRESCALER", {
		.nodes = {{
			{T21, Damaged, Damaged, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{in, null, null, null, }},
		.outputs = {{null, out, out, out, }},
	}},
	{"NEXUS.06.786.0", "SIGNAL AVERAGER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.07.050.0", "SUBMAXIMUM SELECTOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{in, in, in, in, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.08.633.9", "DECIMAL DECOMPOSER", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{out, out, out, null, }},
	}},
	{"NEXUS.09.904.9", "SEQUENCE MODE CALCULATOR", {
		.nodes = {{
			{T30, T21, T30, Damaged, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, out, null, null, }},
	}},
	{"NEXUS.10.656.5", "SEQUENCE NORMALIZER", {
		.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T30, },
			{T21, Damaged, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.11.711.2", "IMAGE TEST PATTERN 3", {
		.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, null, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"NEXUS.12.534.4", "IMAGE TEST PATTERN 4", {
		.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, null, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"NEXUS.13.370.9", "SPATIAL PATH VIEWER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"NEXUS.14.781.3", "CHARACTER TERMINAL", {
		.nodes = {{
			{T30, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T30, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, image, null, }},
	}},
	{"NEXUS.15.897.9", "BACK-REFERENCE REIFIER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.16.212.8", "DYNAMIC PATTERN DETECTOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs = {{in, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.17.135.0", "SEQUENCE GAP INTERPOLATOR", {
		.nodes = {{
			{Damaged, T21, T21, T21, },
			{Damaged, T30, T21, T30, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.18.427.7", "DECIMAL TO OCTAL CONVERTER", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.19.762.9", "PROLONGED SEQUENCE SORTER", {
		.nodes = {{
			{Damaged, T30, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs = {{null, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.20.433.1", "PRIME FACTOR CALCULATOR", {
		.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{null, in, null, null, }},
		.outputs = {{null, out, null, null, }},
	}},
	{"NEXUS.21.601.6", "SIGNAL EXPONENTIATOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{Damaged, T21, T21, T21, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.22.280.8", "T20 NODE EMULATOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}},
		.inputs = {{null, in, in, null, }},
		.outputs = {{null, out, null, null, }},
	}},
	{"NEXUS.23.727.9", "T31 NODE EMULATOR", {
		.nodes = {{
			{Damaged, T30, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}},
		.inputs = {{null, null, in, null, }},
		.outputs = {{null, null, out, null, }},
	}},
	{"NEXUS.24.511.7", "WAVE COLLAPSE SUPERVISOR", {
		.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}},
		.inputs = {{in, in, in, in, }},
		.outputs = {{null, out, null, null, }},
	}},
	}};
}();

#endif // BUILTIN_SPECS_HPP
