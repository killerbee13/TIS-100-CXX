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
#ifndef IO_HPP
#define IO_HPP

#include "node.hpp"
#include <string>
#include <vector>

enum io_type_t { numeric, ascii, list };

struct input_node : node {
	type_t type() const override { return in; }
	void step() override { return; }
	void read() override { return; }

	std::vector<word_t> inputs;
	std::string filename;
	io_type_t io_type;
};
struct output_node : node {
	type_t type() const override { return out; }
	void step() override { return; }
	void read() override { return; }
	std::vector<word_t> outputs_expected;
	std::vector<word_t> outputs_received;
	std::string filename;
	io_type_t io_type;
	char d{' '};
};
struct image_output : node {
	type_t type() const override { return image; }
	void step() override { return; }
	void read() override { return; }

	enum pixel { black, dark_grey, light_grey, white, red };
	std::vector<pixel> image_expected;
	std::vector<pixel> data;
	std::size_t width;
	std::size_t height() const noexcept { return data.size() / width; }
};

#endif // IO_HPP
