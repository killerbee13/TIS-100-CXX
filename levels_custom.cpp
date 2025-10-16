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

#if TIS_ENABLE_LUA

#	include "levels.hpp"

#	include "field.hpp"
#	include "image.hpp"
#	include "logger.hpp"
#	include "node.hpp"
#	include "tests.hpp"
#	include "tis_random.hpp"

#	include <kblib/io.h>
#	include <sol/sol.hpp>

#	include <algorithm>
#	include <filesystem>
#	include <vector>

// this is the fastest way to pull integers from lua if you want them to
// truncate, the magic `.as<vector<...>>` rounds floats
template <typename T>
inline std::vector<T> table_to_vector(const sol::table& table) {
	std::vector<T> ret(table.size());
	for (size_t i = 0; i < ret.size(); i++) {
		ret[i] = T(table.raw_get<double>(i + 1));
	}
	return ret;
}

custom_level::custom_level(std::filesystem::path spec_path)
    : script(kblib::try_get_file_contents(spec_path)) {
	auto spec_filename = spec_path.filename().replace_extension().string();
	if (std::ranges::all_of(spec_filename,
	                        [](char c) { return "0123456789"sv.contains(c); })) {
		base_seed = kblib::parse_integer<uint32_t>(spec_filename);
	} else {
		base_seed = 0;
	}
	init_script();
	spec = layout_from_script(lua);
}

custom_level::custom_level(std::string spec_code, std::uint32_t base_seed_)
    : level(base_seed_)
    , script(std::move(spec_code)) {
	init_script();
	spec = layout_from_script(lua);
}
// avoid having to run get_layout() and get_streams() again in the new context
custom_level::custom_level(std::string spec_code, dynamic_layout_spec spec_,
                           std::uint32_t base_seed_)
    : level(base_seed_)
    , spec(std::move(spec_))
    , script(std::move(spec_code)) {
	init_script();
}
std::unique_ptr<level> custom_level::clone() const {
	return std::make_unique<custom_level>(script, spec, base_seed);
}

void custom_level::init_script() {
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

	// stop custom level authors from messing with the game
	lua["math"]["randomseed"].set_function([](uint32_t) {
		throw std::runtime_error("randomseed() is not allowed in custom levels");
	});

	lua.script(script);
}

dynamic_layout_spec custom_level::layout_from_script(sol::state& lua) {
	dynamic_layout_spec spec;
	std::size_t width = 4;
	if (auto l = lua.get<sol::optional<sol::function>>("get_layout_ext")) {
		log_info("Extended layout spec detected");
		sol::table layout = (*l)();
		auto height = layout.size();
		log_info("Extended layout height: ", height);
		if (height == 0) {
			throw std::invalid_argument(
			    "get_layout_ext(): layout has zero height");
		}
		spec.nodes.resize(height);
		width = layout.get<sol::table>(1).size();
		log_info("Extended layout width: ", width);
		if (width == 0) {
			throw std::invalid_argument("get_layout_ext(): layout has zero width");
		}
		spec.inputs.resize(width, node::null);
		spec.outputs.resize(width, node::null);
		for (auto c : range(height)) {
			spec.nodes[c].resize(width);
			if (layout.get<sol::table>(c + 1).size() != width) {
				throw std::invalid_argument(
				    concat("get_layout_ext(): non-rectangular layout specified, "
				           "line 1 has width ",
				           width, " while line ", c + 1, " has width ",
				           layout.get<sol::table>(c).size()));
			}
			for (auto r : range(width)) {
				spec.nodes[c][r] = layout[c + 1][r + 1];
			}
		}
	} else {
		// unstructured vector, we can only support the default game 4x3 layout
		sol::table layout = lua["get_layout"]();
		if (layout.size() != 12) {
			throw std::invalid_argument{concat(
			    "get_layout(): Given ", layout.size(), " nodes instead of 12")};
		}
		spec.nodes.resize(3, std::vector<node::type_t>(4));
		spec.inputs.resize(4, node::null);
		spec.outputs.resize(4, node::null);
		for (size_t i = 0; i < 12; i++) {
			spec.nodes[i / 4][i % 4] = layout[i + 1];
		}
	}
	// {{STREAM_$TYPE, "$NAME1", <number in [0-3]>, $list1},
	//  {STREAM_$TYPE, "$NAME2", <number in [0-3]>, $list2}, ...}

	// name and values unused for layout purposes
	sol::table streams = lua["get_streams"]();
	for (const auto& [_, stream] : streams) {
		sol::table io = stream.as<sol::table>();
		node::type_t type = io[1];
		uint id = io[3];
		if (id >= width) {
			throw std::runtime_error(
			    concat("get_streams(): io node (type=", to_string(type),
			           ") position ", id, " out of range (width=", width, ")"));
		}
		if (type == node::in) {
			spec.inputs[id] = node::in;
		} else {
			spec.outputs[id] = type;
		}
	}
	return spec;
}

field custom_level::new_field(uint T30_size) const {
	return field(spec, T30_size);
}

std::optional<single_test> custom_level::random_test(std::uint32_t seed) {
	single_test ret;
	ret.inputs.resize(spec.inputs.size());
	ret.n_outputs.resize(spec.outputs.size());
	ret.i_outputs.resize(spec.outputs.size());

	lua_random engine(to_signed(seed));

	lua["math"]["random"].set_function(sol::overload(
	    [&engine] { return engine.next_double(); },
	    [&engine](i32 max) { return engine.lua_next(max); },
	    [&engine](i32 a, i32 b) { return engine.lua_next(a, b); }));
	sol::table streams = lua["get_streams"]();

	for (const auto& [_, stream] : streams) {
		sol::table io = stream.as<sol::table>();
		node::type_t type = io[1];
		uint id = io[3];
		if (id >= spec.nodes[0].size()) {
			throw std::invalid_argument(concat(
			    "get_streams(): io node (type=", to_string(type), ") position ",
			    id, " out of range (width=", spec.nodes[0].size(), ")"));
		}
		sol::table values = io[4];
		switch (type) {
		case node::in: {
			ret.inputs[id] = table_to_vector<word_t>(values);
		} break;
		case node::out: {
			ret.n_outputs[id] = table_to_vector<word_t>(values);
		} break;
		case node::image: {
			ret.i_outputs[id] = image_t(image_width, image_height,
			                            table_to_vector<tis_pixel>(values));
		} break;
		default:
			throw std::invalid_argument{
			    concat("Illegal IO node type ", to_string(type), " in test")};
		}
	}

	for (ptrdiff_t i = spec.inputs.size() - 1; i >= 0; i--) {
		if (spec.inputs[i] != node::in) {
			ret.inputs.erase(ret.inputs.begin() + i);
		}
	}
	for (ptrdiff_t i = spec.outputs.size() - 1; i >= 0; i--) {
		if (spec.outputs[i] != node::out) {
			ret.n_outputs.erase(ret.n_outputs.begin() + i);
		}
		if (spec.outputs[i] != node::image) {
			ret.i_outputs.erase(ret.i_outputs.begin() + i);
		}
	}
	clamp_test_values(ret);
	return ret;
}

#endif // TIS_ENABLE_LUA
