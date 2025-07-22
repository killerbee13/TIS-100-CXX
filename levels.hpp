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

#include "builtin_specs.hpp"
#include "score.hpp"
#include "tests.hpp"
#include "utils.hpp"

#include <cassert>
#include <mutex>
#include <string>

#if TIS_ENABLE_LUA
#	include <sol/sol.hpp>
#endif

class field;

struct level {
	std::uint32_t base_seed;

	virtual field new_field(uint T30_size) const = 0;

	virtual std::optional<single_test> random_test(std::uint32_t seed) = 0;

	single_test static_test(uint id) {
		assert(id < 3);
		// static tests never fail to generate
		return *random_test(base_seed * 100 + id);
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

struct builtin_level final : level {
	uint level_id;

	builtin_level(uint builtin_level_id) {
		level_id = builtin_level_id;
		base_seed = builtin_seeds[level_id];
	}

	field new_field(uint T30_size) const override;

	std::optional<single_test> random_test(std::uint32_t seed) override {
		return gen_random_test(level_id, seed);
	}

	bool has_achievement(const field& solve, const score& sc) const override;
};

#if TIS_ENABLE_LUA

struct custom_level final : level {
	dynamic_layout_spec spec;
	sol::state lua;
	/// sol::state is not thread-safe
	std::mutex lua_mutex;

	custom_level(const std::string& spec_path);

	field new_field(uint T30_size) const override;

	std::optional<single_test> random_test(std::uint32_t seed) override;

	bool has_achievement(const field&, const score&) const override {
		return false;
	}
};

#endif

#endif // LEVELS_HPP
