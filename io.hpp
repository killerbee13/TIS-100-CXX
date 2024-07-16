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

#include "logger.hpp"
#include "node.hpp"
#include <string>
#include <vector>

enum io_type_t { numeric, ascii, list };

struct input_node : node {
	using node::node;
	type_t type() const noexcept override { return in; }
	bool step() override { return false; }
	bool finalize() override {
		if (not wrt and idx != inputs.size()) {
			log_debug("I: ", not wrt, ',', idx != inputs.size());
			wrt.emplace(inputs[idx++]);
			return true;
		} else {
			return false;
		}
	}
	std::optional<word_t> read_(port) override {
		return std::exchange(wrt, std::nullopt);
	}

	std::vector<word_t> inputs;
	std::size_t idx{};
	std::string filename{};
	io_type_t io_type{};
	std::optional<word_t> wrt;
};
struct output_node : node {
	using node::node;
	type_t type() const noexcept override { return out; }
	bool step() override {
		if (auto r = do_read(neighbors[port::up], port::down)) {
			log_debug("O: read");
			outputs_received.push_back(*r);
			return true;
		} else {
			return false;
		}
	}
	bool finalize() override { return false; }
	std::optional<word_t> read_(port) override { return std::nullopt; }
	std::vector<word_t> outputs_expected;
	std::vector<word_t> outputs_received;
	std::string filename;
	io_type_t io_type;
	char d{' '};
};
struct image_output : node {
	using node::node;
	type_t type() const noexcept override { return image; }
	bool step() override { return false; }
	bool finalize() override { return false; }
	std::optional<word_t> read_(port) override { return std::nullopt; }

	image_t image_expected;
	image_t image_received;
	std::string filename;
	std::size_t width;
	std::size_t height;
};

#endif // IO_HPP
