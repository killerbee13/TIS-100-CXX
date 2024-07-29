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

#include "image.hpp"

std::error_code extract_comment_from_file(const char* filename,
                                          std::string& out) {
	using namespace std::literals;
	auto pipename
	    = std::filesystem::temp_directory_path()
	      / concat(
	          kblib::to_string(
	              std::chrono::system_clock::now().time_since_epoch().count(),
	              16),
	          '-', filename);
	auto status = mkfifo(pipename.c_str(), S_IRUSR | S_IWUSR);
	if (status) {
		return pnm::errno_to_error_code();
	}
	auto _ = kblib::defer([&] { std::filesystem::remove(pipename); });

	if (auto pid = fork(); pid > 0) {
		// parent
		std::ifstream output(pipename);
		if (not output) {
			return std::make_error_code(std::errc::io_error);
		}
		kblib::get_contents(output, out);
		if (output) {
			return {};
		} else {
			return std::error_code(std::io_errc::stream);
		}
	} else if (pid == 0) {
		// child

		auto p = pipename.native();
		char cmd[] = "identify";
		char format_arg[] = "-format";
		char format[] = "%[comment]";
		char* cargs[] = {cmd, format_arg, format, p.data(), nullptr};

		auto [convert_path, ec] = pnm::find_in_path(cmd);
		if (ec) {
			std::cerr << "Could not find 'convert'\n";
			return ec;
		}

		execv(convert_path.c_str(), cargs);
		std::cerr << "Exec failure: " << pnm::errno_to_message() << '\n';
		std::ifstream in(pipename);
		while (in) {
			in.ignore(kblib::max);
		}
		in.close();
		abort();
	} else {
		// failed to fork()
		std::cerr << "fork failure: " << pnm::errno_to_message() << '\n';
		return pnm::errno_to_error_code();
	}
}
