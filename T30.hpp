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
#ifndef T30_HPP
#define T30_HPP

#include "node.hpp"

#include <vector>

struct T30 : node {
	using node::node;
	word_vec data;
	word_vec buffer;
	std::size_t max_size{def_T30_size};
	bool wrote{};
	bool used{};
	type_t type() const noexcept override { return type_t::T30; }
	void step() override {
		if (data.size() == max_size) {
			return;
		}
		for (auto p : {port::left, port::right, port::up, port::down, port::D5,
		               port::D6}) {
			if (auto r
			    = do_read(neighbors[static_cast<std::size_t>(p)], invert(p))) {
				buffer.push_back(*r);
				used = true;
				if (data.size() + buffer.size() == max_size) {
					break;
				}
			}
		}
	}
	void finalize() override {
		data.insert(data.end(), buffer.begin(), buffer.end());
		buffer.clear();
		wrote = false;
	}
	void reset() noexcept override {
		data.clear();
		buffer.clear();
		wrote = false;
	}
	std::optional<word_t> emit(port) override {
		if (not wrote and not data.empty()) {
			auto v = data.back();
			data.pop_back();
			wrote = true;
			return v;
		} else {
			return std::nullopt;
		}
	}
	std::string print() const override {
		std::string ret = concat('(', x, ',', y, ") T30 {");
		for (auto w : data) {
			append(ret, w, ", ");
		}
		append(ret, '}');
		return ret;
	}
};

#endif // T30_HPP
