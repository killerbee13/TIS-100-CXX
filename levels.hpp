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
#ifndef LEVELS_HPP
#define LEVELS_HPP

#include "T21.hpp"
#include "T30.hpp"
#include "builtin_specs.hpp"
#include "field.hpp"
#include "image.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "tis_random.hpp"
#include "utils.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

#if TIS_ENABLE_LUA
#	include <sol/sol.hpp>
#endif

struct level {
	std::uint32_t base_seed;

	virtual field new_field(uint T30_size) = 0;

	virtual std::optional<single_test> random_test(std::uint32_t seed) = 0;

	std::array<single_test, 3> static_suite() {
		return {
		    *random_test(base_seed * 100),
		    *random_test(base_seed * 100 + 1),
		    *random_test(base_seed * 100 + 2),
		};
	}

	virtual bool has_achievement(const field& f, const score& sc) const = 0;

	virtual ~level() = default;

 protected: // prevents most slicing
	level() = default;
	level(const level&) = default;
	level(level&&) = default;
	level& operator=(const level&) = default;
	level& operator=(level&&) = default;
};

inline constexpr uint find_level_id(std::string_view s) {
	for (auto i : range(builtin_layouts.size())) {
		if (s == builtin_layouts[i].segment or s == builtin_layouts[i].name) {
			return static_cast<uint>(i);
		}
	}
	throw std::invalid_argument{concat("invalid level ID ", kblib::quoted(s))};
}

inline consteval uint operator""_lvl(const char* s, std::size_t size) {
	return find_level_id(std::string_view(s, size));
}

inline constexpr std::optional<uint> guess_level_id(std::string_view filename) {
	for (auto i : range(builtin_layouts.size())) {
		if (filename.starts_with(builtin_layouts[i].segment)) {
			return i;
		}
	}
	return std::nullopt;
}

struct builtin_level final : level {
	uint level_id;

	builtin_level(uint builtin_level_id) {
		level_id = builtin_level_id;
		base_seed = builtin_seeds[level_id];
	}

	field new_field(uint T30_size) override {
		return field(builtin_layouts[level_id].layout, T30_size);
	}

	std::optional<single_test> random_test(std::uint32_t seed) override;

	bool has_achievement(const field& solve, const score& sc) const override {
		auto log = log_debug();
		log << "check_achievement " << builtin_layouts[level_id].name << ": ";
		// SELF-TEST DIAGNOSTIC
		if (level_id == "00150"_lvl) {
			// BUSY_LOOP
			log << "BUSY_LOOP: " << sc.cycles
			    << ((sc.cycles > 100000) ? ">" : "<=") << 100000;
			return sc.cycles > 100000;
			// SIGNAL COMPARATOR
		} else if (level_id == "21340"_lvl) {
			// UNCONDITIONAL
			log << "UNCONDITIONAL:\n";
			for (auto& n : solve.regulars()) {
				if (n->type() == node::T21) {
					auto p = static_cast<const T21*>(n.get());
					log << "T20 (" << p->x << ',' << p->y << "): ";
					if (p->code.empty()) {
						log << "empty";
					} else if (p->has_instr(instr::jez, instr::jnz, instr::jgz,
					                        instr::jlz)) {
						log << " conditional found";
						return false;
					}
					log << '\n';
				}
			}
			log << " no conditionals found";
			return true;
			// SEQUENCE REVERSER
		} else if (level_id == "42656"_lvl) {
			// NO_MEMORY
			log << "NO_MEMORY: ";
			for (auto& n : solve.regulars()) {
				if (n->type() == node::T30) {
					auto p = static_cast<const T30*>(n.get());
					log << "T30 (" << p->x << ',' << p->y << "): " << p->used
					    << '\n';
					if (p->used) {
						return false;
					}
				}
			}
			log << "no stacks used";
			return true;
		} else {
			log << "no achievement";
			return false;
		}
	}
};

#if TIS_ENABLE_LUA

// this is the fastest way to pull integers from lua if you want them to
// truncate, the magic `.as<vector<...>>` rounds floats
template <typename T>
inline std::vector<T> table_to_vector(const sol::table& table) {
	std::vector<T> ret;
	ret.resize(table.size());
	for (size_t i = 0; i < ret.size(); i++) {
		ret[i] = T(table.raw_get<double>(i + 1));
	}
	return ret;
}

struct custom_level final : level {
	standard_layout_spec spec;
	sol::state lua;

	custom_level(const std::string& spec_path) {
		auto spec_filename = std::filesystem::path(spec_path)
		                         .filename()
		                         .replace_extension()
		                         .string();
		if (std::ranges::all_of(spec_filename, [](char c) {
			    return "0123456789"sv.contains(c);
		    })) {
			base_seed = kblib::parse_integer<uint32_t>(spec_filename);
		} else {
			base_seed = 0;
		}

		// the game uses MoonSharp's hard sandbox, we open a subset
		// see https://www.moonsharp.org/sandbox.html
		lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math,
		                   sol::lib::table, sol::lib::bit32);
		// predefined node type constants, mapped to ours
		lua["TILE_COMPUTE"] = node::T21;
		lua["TILE_MEMORY"] = node::T30;
		lua["TILE_DAMAGED"] = node::Damaged;
		lua["TILE_JOURNAL"] = node::Damaged;
		lua["STREAM_INPUT"] = node::in;
		lua["STREAM_OUTPUT"] = node::out;
		lua["STREAM_IMAGE"] = node::image;
		lua.script_file(spec_path);

		// unstructured vector, we can only support the default game 4x3 layout
		sol::table layout = lua["get_layout"]();
		if (layout.size() != 12) {
			throw std::invalid_argument{
			    concat("Given ", layout.size(), " nodes instead of 12")};
		}
		for (size_t i = 0; i < 12; i++) {
			spec.nodes[i / 4][i % 4] = layout[i + 1];
		}

		spec.inputs.fill(node::null);
		spec.outputs.fill(node::null);
		// {{STREAM_$TYPE, "$NAME1", <number in [0-3]>, $list1},
		//  {STREAM_$TYPE, "$NAME2", <number in [0-3]>, $list2}, ...}
		sol::table streams = lua["get_streams"]();
		for (const auto& [_, stream] : streams) {
			sol::table io = stream.as<sol::table>();
			node::type_t type = io[1];
			uint id = io[3];
			if (type == node::in) {
				spec.inputs[id] = node::in;
			} else {
				spec.outputs[id] = type;
			}
		}
	}

	field new_field(uint T30_size) override { return field(spec, T30_size); }

	std::optional<single_test> random_test(std::uint32_t seed) override {
		single_test ret;
		ret.inputs.resize(spec.inputs.size());
		ret.n_outputs.resize(spec.outputs.size());

		lua_random engine(to_signed(seed));

		lua["math"]["random"].set_function(&lua_random::next, engine);
		sol::table streams = lua["get_streams"]();

		for (const auto& [_, stream] : streams) {
			sol::table io = stream.as<sol::table>();
			node::type_t type = io[1];
			uint id = io[3];
			sol::table values = io[4];
			switch (type) {
			case node::in: {
				ret.inputs[id] = table_to_vector<word_t>(values);
			} break;
			case node::out: {
				ret.n_outputs[id] = table_to_vector<word_t>(values);
			} break;
			case node::image: {
				ret.i_output.reshape(image_width, image_height);
				ret.i_output.assign(table_to_vector<tis_pixel>(values));
			} break;
			default:
				throw std::invalid_argument{std::to_string(etoi(type))};
			}
		}

		for (ssize_t i = spec.inputs.size() - 1; i >= 0; i--) {
			if (spec.inputs[i] != node::in) {
				ret.inputs.erase(ret.inputs.begin() + i);
			}
		}
		for (ssize_t i = spec.outputs.size() - 1; i >= 0; i--) {
			if (spec.outputs[i] != node::out) {
				ret.n_outputs.erase(ret.n_outputs.begin() + i);
			}
		}
		clamp_test_values(ret);
		return ret;
	}

	bool has_achievement(const field&, const score&) const override {
		return false;
	}
};
#endif

#endif // LEVELS_HPP
