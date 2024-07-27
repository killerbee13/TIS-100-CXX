#ifndef LAYOUTSPECS_HPP
#define LAYOUTSPECS_HPP

#include "node.hpp"

struct builtin_layout_spec {
	struct io_node_spec {
		node::type_t type;
		std::optional<std::pair<word_t, word_t>> image_size;
	};

	std::array<std::array<node::type_t, 4>, 3> nodes;
	std::array<std::array<io_node_spec, 4>, 2> io;
};

struct level_layout1 {
	std::string_view segment;
	std::string_view name;
	builtin_layout_spec layout;
};

constexpr std::array<level_layout1, 51> gen_layouts() {
	using enum node::type_t;
	return {{
	{"00150", "SELF-TEST DIAGNOSTIC", {.nodes = {{
		{T21, Damaged, T21, T21, },
		{T21, Damaged, T21, Damaged, },
		{T21, Damaged, T21, T21, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = null},
			{.type = null},
			{.type = in},
		}},
		{{
			{.type = out},
			{.type = null},
			{.type = null},
			{.type = out},
		}},
	}}}},
	{"10981", "SIGNAL AMPLIFIER", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"20176", "DIFFERENTIAL CONVERTER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"21340", "SIGNAL COMPARATOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, Damaged, Damaged, Damaged, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = null},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = out},
		}},
	}}}},
	{"22280", "SIGNAL MULTIPLEXER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = in},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"30647", "SEQUENCE GENERATOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, Damaged, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"31904", "SEQUENCE COUNTER", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"32050", "SIGNAL EDGE DETECTOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"33762", "INTERRUPT HANDLER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = in},
			{.type = in},
			{.type = in},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"USEG0", "SIMPLE SANDBOX", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"40196", "SIGNAL PATTERN DETECTOR", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"41427", "SEQUENCE PEAK DETECTOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"42656", "SEQUENCE REVERSER", {.nodes = {{
		{T21, T21, T30, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T30, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"43786", "SIGNAL MULTIPLIER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T30, T21, T21, T30, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"USEG1", "STACK MEMORY SANDBOX", {.nodes = {{
		{T21, T21, T30, T21, },
		{T21, T21, T21, T21, },
		{T21, T30, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"50370", "IMAGE TEST PATTERN 1", {.nodes = {{
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = null},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"51781", "IMAGE TEST PATTERN 2", {.nodes = {{
		{Damaged, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = null},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"52544", "EXPOSURE MASK VIEWER", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"53897", "HISTOGRAM VIEWER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"USEG2", "IMAGE CONSOLE SANDBOX", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{36, 22}}},
			{.type = null},
		}},
	}}}},
	{"60099", "SIGNAL WINDOW FILTER", {.nodes = {{
		{Damaged, T21, T21, T30, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T30, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"61212", "SIGNAL DIVIDER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T30, T21, T21, T30, },
		{T21, T21, T21, Damaged, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"62711", "SEQUENCE INDEXER", {.nodes = {{
		{T21, T30, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T30, T21, T21, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = null},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"63534", "SEQUENCE SORTER", {.nodes = {{
		{Damaged, T21, T30, T21, },
		{T21, T21, T21, T21, },
		{T21, T30, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"70601", "STORED IMAGE DECODER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"UNKNOWN", "UNKNOWN", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, Damaged, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.00.526.6", "SEQUENCE MERGER", {.nodes = {{
		{T21, T21, Damaged, T21, },
		{T30, T21, T21, T21, },
		{T21, T21, T21, T30, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = in},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.01.874.8", "INTEGER SERIES CALCULATOR", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, Damaged, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = null},
			{.type = null},
		}},
	}}}},
	{"NEXUS.02.981.2", "SEQUENCE RANGE LIMITER", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = null},
			{.type = null},
		}},
	}}}},
	{"NEXUS.03.176.9", "SIGNAL ERROR CORRECTOR", {.nodes = {{
		{Damaged, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.04.340.5", "SUBSEQUENCE EXTRACTOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.05.647.1", "SIGNAL PRESCALER", {.nodes = {{
		{T21, Damaged, Damaged, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = null},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = out},
			{.type = out},
		}},
	}}}},
	{"NEXUS.06.786.0", "SIGNAL AVERAGER", {.nodes = {{
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.07.050.0", "SUBMAXIMUM SELECTOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = in},
			{.type = in},
			{.type = in},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.08.633.9", "DECIMAL DECOMPOSER", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = out},
			{.type = out},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.09.904.9", "SEQUENCE MODE CALCULATOR", {.nodes = {{
		{T30, T21, T30, Damaged, },
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, Damaged, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = null},
			{.type = null},
		}},
	}}}},
	{"NEXUS.10.656.5", "SEQUENCE NORMALIZER", {.nodes = {{
		{T21, T21, T30, T21, },
		{T21, T21, T21, T30, },
		{T21, Damaged, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.11.711.2", "IMAGE TEST PATTERN 3", {.nodes = {{
		{Damaged, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = null},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"NEXUS.12.534.4", "IMAGE TEST PATTERN 4", {.nodes = {{
		{Damaged, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = null},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"NEXUS.13.370.9", "SPATIAL PATH VIEWER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"NEXUS.14.781.3", "CHARACTER TERMINAL", {.nodes = {{
		{T30, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T30, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = image, .image_size = {{30, 18}}},
			{.type = null},
		}},
	}}}},
	{"NEXUS.15.897.9", "BACK-REFERENCE REIFIER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.16.212.8", "DYNAMIC PATTERN DETECTOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, Damaged, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = null},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.17.135.0", "SEQUENCE GAP INTERPOLATOR", {.nodes = {{
		{Damaged, T21, T21, T21, },
		{Damaged, T30, T21, T30, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = null},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.18.427.7", "DECIMAL TO OCTAL CONVERTER", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.19.762.9", "PROLONGED SEQUENCE SORTER", {.nodes = {{
		{Damaged, T30, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T30, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = null},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.20.433.1", "PRIME FACTOR CALCULATOR", {.nodes = {{
		{T21, T21, T21, Damaged, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = null},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = null},
			{.type = null},
		}},
	}}}},
	{"NEXUS.21.601.6", "SIGNAL EXPONENTIATOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T30, T21, T21, T30, },
		{Damaged, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.22.280.8", "T20 NODE EMULATOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, Damaged, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = in},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = null},
			{.type = null},
		}},
	}}}},
	{"NEXUS.23.727.9", "T31 NODE EMULATOR", {.nodes = {{
		{Damaged, T30, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T30, T21, T21, },
	}}, .io = {{
		{{
			{.type = null},
			{.type = null},
			{.type = in},
			{.type = null},
		}},
		{{
			{.type = null},
			{.type = null},
			{.type = out},
			{.type = null},
		}},
	}}}},
	{"NEXUS.24.511.7", "WAVE COLLAPSE SUPERVISOR", {.nodes = {{
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
		{T21, T21, T21, T21, },
	}}, .io = {{
		{{
			{.type = in},
			{.type = in},
			{.type = in},
			{.type = in},
		}},
		{{
			{.type = null},
			{.type = out},
			{.type = null},
			{.type = null},
		}},
	}}}},
}};
}

inline constexpr auto layouts1 = gen_layouts();

#endif
