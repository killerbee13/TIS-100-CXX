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
#ifndef LEVELS_HPP
#define LEVELS_HPP

#include "game.hpp"
#include "tests.hpp"
#include "tis100.h"
#include "utils.hpp"

#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#if TIS_ENABLE_LUA
#	include <filesystem>
#	include <sol/sol.hpp>
#endif

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

	// constructs a level equivalent to this immediately after construction
	virtual std::unique_ptr<level> clone() const = 0;

	virtual ~level() = default;

 protected: // prevents most slicing
	level() = default;
	constexpr level(std::uint32_t base_seed_)
	    : base_seed(base_seed_) {}
	level(const level&) = default;
	level(level&&) = default;
	level& operator=(const level&) = default;
	level& operator=(level&&) = default;
};

struct builtin_level final : level {
	standard_layout_spec layout;
	std::string_view segment;
	std::string_view name;

	using test_producer_t = std::optional<single_test>(std::uint32_t);
	test_producer_t* test_producer;

	constexpr builtin_level(std::string_view segment_, std::string_view name_,
	                        std::uint32_t base_seed_,
	                        standard_layout_spec layout_,
	                        test_producer_t* test_producer_)
	    : level(base_seed_)
	    , layout(layout_)
	    , segment(segment_)
	    , name(name_)
	    , test_producer(test_producer_) {}

	static std::unique_ptr<builtin_level> from_name(std::string_view s);

	std::unique_ptr<level> clone() const override;

	field new_field(uint T30_size) const override;

	std::optional<single_test> random_test(std::uint32_t seed) override {
		return (*test_producer)(seed);
	}
	bool has_achievement(const field& solve, const score& sc) const override;
};

inline constexpr size_t builtin_levels_num = 51;
extern const std::array<builtin_level, builtin_levels_num> builtin_levels;

#if TIS_ENABLE_LUA

struct custom_level final : level {
	dynamic_layout_spec spec;

	custom_level(std::filesystem::path spec_path);
	custom_level(std::string spec_code, std::uint32_t base_seed_);
	custom_level(std::string spec_code, dynamic_layout_spec spec_,
	             std::uint32_t base_seed_);
	std::unique_ptr<level> clone() const override;

	field new_field(uint T30_size) const override;

	std::optional<single_test> random_test(std::uint32_t seed) override;

	bool has_achievement(const field&, const score&) const override {
		return false;
	}

 private:
	std::string script;
	sol::state lua;

	static dynamic_layout_spec layout_from_script(sol::state& lua);
	void init_script();
};

#endif

#endif // LEVELS_HPP
