#ifndef LAYOUTSPECS_HPP
#define LAYOUTSPECS_HPP

#include "node.hpp"

struct builtin_layout_spec {
	struct io_node_spec {
		node::type_t type;
		std::optional<std::pair<word_t, word_t>> image_size{};
	};

	std::array<std::array<node::type_t, 4>, 3> nodes;
	std::array<std::array<io_node_spec, 4>, 2> io;
};

struct level_layout {
	std::string_view segment;
	std::string_view name;
	builtin_layout_spec layout;
};

// clang-format off

constexpr std::array<level_layout, 51> gen_layouts() {
	using enum node::type_t;
	return {{
	{"00150", "SELF-TEST DIAGNOSTIC",
		{.nodes = {{
			{T21, Damaged, T21, T21, },
			{T21, Damaged, T21, Damaged, },
			{T21, Damaged, T21, T21, },
		}}, .io = {{
			{{{in, {}}, {null, {}}, {null, {}}, {in, {}}, }},
			{{{out, {}}, {null, {}}, {null, {}}, {out, {}}, }},
	}}}},
	{"10981", "SIGNAL AMPLIFIER",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"20176", "DIFFERENTIAL CONVERTER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"21340", "SIGNAL COMPARATOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, Damaged, Damaged, Damaged, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{in, {}}, {null, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {out, {}}, }},
	}}}},
	{"22280", "SIGNAL MULTIPLEXER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {in, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"30647", "SEQUENCE GENERATOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, Damaged, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"31904", "SEQUENCE COUNTER",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"32050", "SIGNAL EDGE DETECTOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"33762", "INTERRUPT HANDLER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{in, {}}, {in, {}}, {in, {}}, {in, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"USEG0", "SIMPLE SANDBOX",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"40196", "SIGNAL PATTERN DETECTOR",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"41427", "SEQUENCE PEAK DETECTOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"42656", "SEQUENCE REVERSER",
		{.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T30, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"43786", "SIGNAL MULTIPLIER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"USEG1", "STACK MEMORY SANDBOX",
		{.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"50370", "IMAGE TEST PATTERN 1",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {null, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"51781", "IMAGE TEST PATTERN 2",
		{.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {null, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"52544", "EXPOSURE MASK VIEWER",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"53897", "HISTOGRAM VIEWER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"USEG2", "IMAGE CONSOLE SANDBOX",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{36, 22}}}, {null, {}}, }},
	}}}},
	{"60099", "SIGNAL WINDOW FILTER",
		{.nodes = {{
			{Damaged, T21, T21, T30, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T30, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"61212", "SIGNAL DIVIDER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{T21, T21, T21, Damaged, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"62711", "SEQUENCE INDEXER",
		{.nodes = {{
			{T21, T30, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}}, .io = {{
			{{{in, {}}, {null, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"63534", "SEQUENCE SORTER",
		{.nodes = {{
			{Damaged, T21, T30, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"70601", "STORED IMAGE DECODER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"UNKNOWN", "UNKNOWN",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, Damaged, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.00.526.6", "SEQUENCE MERGER",
		{.nodes = {{
			{T21, T21, Damaged, T21, },
			{T30, T21, T21, T21, },
			{T21, T21, T21, T30, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {in, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.01.874.8", "INTEGER SERIES CALCULATOR",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {null, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.02.981.2", "SEQUENCE RANGE LIMITER",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{in, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {null, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.03.176.9", "SIGNAL ERROR CORRECTOR",
		{.nodes = {{
			{Damaged, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.04.340.5", "SUBSEQUENCE EXTRACTOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.05.647.1", "SIGNAL PRESCALER",
		{.nodes = {{
			{T21, Damaged, Damaged, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{in, {}}, {null, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {out, {}}, {out, {}}, }},
	}}}},
	{"NEXUS.06.786.0", "SIGNAL AVERAGER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.07.050.0", "SUBMAXIMUM SELECTOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{in, {}}, {in, {}}, {in, {}}, {in, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.08.633.9", "DECIMAL DECOMPOSER",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{out, {}}, {out, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.09.904.9", "SEQUENCE MODE CALCULATOR",
		{.nodes = {{
			{T30, T21, T30, Damaged, },
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, Damaged, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {null, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.10.656.5", "SEQUENCE NORMALIZER",
		{.nodes = {{
			{T21, T21, T30, T21, },
			{T21, T21, T21, T30, },
			{T21, Damaged, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.11.711.2", "IMAGE TEST PATTERN 3",
		{.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {null, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"NEXUS.12.534.4", "IMAGE TEST PATTERN 4",
		{.nodes = {{
			{Damaged, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {null, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"NEXUS.13.370.9", "SPATIAL PATH VIEWER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"NEXUS.14.781.3", "CHARACTER TERMINAL",
		{.nodes = {{
			{T30, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T30, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {image, {{30, 18}}}, {null, {}}, }},
	}}}},
	{"NEXUS.15.897.9", "BACK-REFERENCE REIFIER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.16.212.8", "DYNAMIC PATTERN DETECTOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}}, .io = {{
			{{{in, {}}, {null, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.17.135.0", "SEQUENCE GAP INTERPOLATOR",
		{.nodes = {{
			{Damaged, T21, T21, T21, },
			{Damaged, T30, T21, T30, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {null, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.18.427.7", "DECIMAL TO OCTAL CONVERTER",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.19.762.9", "PROLONGED SEQUENCE SORTER",
		{.nodes = {{
			{Damaged, T30, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {null, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.20.433.1", "PRIME FACTOR CALCULATOR",
		{.nodes = {{
			{T21, T21, T21, Damaged, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {null, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {null, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.21.601.6", "SIGNAL EXPONENTIATOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T30, T21, T21, T30, },
			{Damaged, T21, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.22.280.8", "T20 NODE EMULATOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, Damaged, },
		}}, .io = {{
			{{{null, {}}, {in, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {out, {}}, {null, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.23.727.9", "T31 NODE EMULATOR",
		{.nodes = {{
			{Damaged, T30, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T30, T21, T21, },
		}}, .io = {{
			{{{null, {}}, {null, {}}, {in, {}}, {null, {}}, }},
			{{{null, {}}, {null, {}}, {out, {}}, {null, {}}, }},
	}}}},
	{"NEXUS.24.511.7", "WAVE COLLAPSE SUPERVISOR",
		{.nodes = {{
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
			{T21, T21, T21, T21, },
		}}, .io = {{
			{{{in, {}}, {in, {}}, {in, {}}, {in, {}}, }},
			{{{null, {}}, {out, {}}, {null, {}}, {null, {}}, }},
	}}}},
}};
}

inline constexpr std::array<level_layout, 51> layouts = gen_layouts();

#ifdef OLD_LAYOUTS
#warning "hi"
struct level_layout_s {
	std::string_view segment;
	std::string_view name;
	std::string_view layout;
};

inline constexpr std::array<level_layout_s, 51> layouts{{
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
     "BBBBBBBBCBBB I0 NUMERIC @0 I1 NUMERIC @1 I2 NUMERIC @2 I3 "
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
#endif

#endif
