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
	std::vector<word_t> data;
	std::size_t max_size{def_T30_size};
	bool read{};
	bool wrote{};
	type_t type() const noexcept override { return type_t::T30; }
	bool step() override {
		read = false;
		if (not wrote and data.size() < max_size) {
			for (auto p : {port::up, port::left, port::right, port::down, port::D5,
			               port::D6}) {
				if (auto r
				    = do_read(neighbors[static_cast<std::size_t>(p)], invert(p))) {
					data.push_back(*r);
					return true;
				}
			}
		}
		return false;
	}
	bool finalize() override {
		wrote = false;
		return false;
	}
	void reset() noexcept override {
		data.clear();
		read = false;
		wrote = false;
	}
	std::optional<word_t> read_(port) override {
		if (not read and not data.empty()) {
			auto v = data.back();
			data.pop_back();
			wrote = true;
			return v;
		} else {
			return std::nullopt;
		}
	}
	std::string print() const override {
		std::string ret = kblib::concat('(', x, ',', y, ") T30 {");
		for (auto w : data) {
			kblib::append(ret, w, ", ");
		}
		kblib::append(ret, '}');
		return ret;
	}
};

#endif // T30_HPP
