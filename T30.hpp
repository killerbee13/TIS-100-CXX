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
#ifndef T30_HPP
#define T30_HPP

#include "node.hpp"
#include "utils.hpp"

struct T30 final : regular_node {
	T30(int x, int y, std::size_t max_size)
	    : regular_node(x, y)
	    , max_size(max_size) {
		data.reserve(max_size);
	}
	void reset() noexcept {
		data.clear();
		division = 0;
		wrote = false;
	}

	type_t type() const noexcept override { return type_t::T30; }
	void step(logger&) override {
		if (data.size() == max_size) {
			return;
		}
		for (auto p = port::dir_first; p <= port::dir_last; p++) {
			if (auto r = do_read(p); r != word_empty) {
				data.push_back(r);
				used = true;
				if (data.size() == max_size) {
					break;
				}
			}
		}
	}
	void finalize(logger&) override {
		division = data.size();
		wrote = false;
	}
	std::unique_ptr<regular_node> clone() const override {
		return std::make_unique<T30>(x, y, max_size);
	}
	optional_word emit(port) override {
		if (not wrote and division != 0) {
			auto v = data[--division];
			data.erase(data.begin() + to_signed(division));
			wrote = true;
			return v;
		} else {
			return word_empty;
		}
	}
	std::string state() const override {
		std::string ret = concat('(', x, ',', y, ") T30 {");
		for (auto w : data) {
			append(ret, w, ", ");
		}
		append(ret, '}');
		return ret;
	}

	bool used{}; // persistent among all tests

 private:
	word_vec data;
	std::size_t division{};
	std::size_t max_size{def_T30_size};
	bool wrote{};
};

#endif // T30_HPP
