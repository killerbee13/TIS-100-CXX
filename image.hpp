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
#ifndef IMAGE_HPP
#define IMAGE_HPP
#include <kblib/convert.h>
#include <kblib/fakestd.h>
#include <kblib/io.h>
#include <kblib/simple.h>
#include <kblib/stats.h>
#include <kblib/stringops.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <ostream>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace pnm {

struct rgba_pixel {
	std::uint8_t rgb[4];

	constexpr std::uint8_t& operator[](std::size_t i) noexcept { return rgb[i]; }
	constexpr const std::uint8_t& operator[](std::size_t i) const noexcept {
		return rgb[i];
	}

	constexpr std::uint8_t& red() noexcept { return rgb[0]; }
	constexpr const std::uint8_t& red() const noexcept { return rgb[0]; }
	constexpr std::uint8_t& green() noexcept { return rgb[1]; }
	constexpr const std::uint8_t& green() const noexcept { return rgb[1]; }
	constexpr std::uint8_t& blue() noexcept { return rgb[2]; }
	constexpr const std::uint8_t& blue() const noexcept { return rgb[2]; }
	constexpr std::uint8_t& alpha() noexcept { return rgb[3]; }
	constexpr const std::uint8_t& alpha() const noexcept { return rgb[3]; }

	constexpr std::uint8_t value() const noexcept {
		auto R = red() / 255., G = green() / 255., B = blue() / 255.;
		auto Clinear = 0.2126 * R + 0.7152 * G + 0.0722 * B;
		auto Csrgb = [&] {
			if (Clinear <= 0.0031308) {
				return 12.92 * Clinear;
			} else {
				return 1.055 * std::pow(Clinear, 1 / 2.4) - 0.055;
			}
		}();
		return static_cast<std::uint8_t>(255 * Csrgb);
	}
	static auto& header(std::ostream& os, std::ptrdiff_t width,
	                    std::ptrdiff_t height) noexcept {
		return os << "P7\n"
		             "WIDTH "
		          << width
		          << "\n"
		             "HEIGHT "
		          << height
		          << "\n"
		             "DEPTH 4\n"
		             "MAXVAL 255\n"
		             "TUPLTYPE RGB_ALPHA\n"
		             "ENDHDR\n";
	}
	static std::filesystem::path extension() { return ".pam"; }
	friend std::ostream& operator<<(std::ostream& os, const rgba_pixel& px) {
		return os.write(reinterpret_cast<const char*>(px.rgb), 4);
	}
	constexpr static bool is_binary_writable = true;

	constexpr friend std::strong_ordering operator<=>(rgba_pixel, rgba_pixel)
	    = default;
};

struct rgb_pixel {
	std::uint8_t rgb[3];

	constexpr std::uint8_t& operator[](std::size_t i) noexcept { return rgb[i]; }
	constexpr const std::uint8_t& operator[](std::size_t i) const noexcept {
		return rgb[i];
	}

	constexpr std::uint8_t& red() noexcept { return rgb[0]; }
	constexpr const std::uint8_t& red() const noexcept { return rgb[0]; }
	constexpr std::uint8_t& green() noexcept { return rgb[1]; }
	constexpr const std::uint8_t& green() const noexcept { return rgb[1]; }
	constexpr std::uint8_t& blue() noexcept { return rgb[2]; }
	constexpr const std::uint8_t& blue() const noexcept { return rgb[2]; }

	constexpr std::uint8_t value() const noexcept {
		auto R = red() / 255., G = green() / 255., B = blue() / 255.;
		auto Clinear = 0.2126 * R + 0.7152 * G + 0.0722 * B;
		auto Csrgb = [&] {
			if (Clinear <= 0.0031308) {
				return 12.92 * Clinear;
			} else {
				return 1.055 * std::pow(Clinear, 1 / 2.4) - 0.055;
			}
		}();
		return static_cast<std::uint8_t>(255 * Csrgb);
	}
	static auto& header(std::ostream& os, std::ptrdiff_t width,
	                    std::ptrdiff_t height) noexcept {
		return os << "P6" << ' ' << width << ' ' << height << " 255\n";
	}
	static std::filesystem::path extension() { return ".ppm"; }
	friend std::ostream& operator<<(std::ostream& os, const rgb_pixel& px) {
		return os.write(reinterpret_cast<const char*>(px.rgb), 3);
	}
	constexpr static bool is_binary_writable = true;

	constexpr operator rgba_pixel() const noexcept {
		return {red(), green(), blue(), 255};
	}
	constexpr friend std::strong_ordering operator<=>(rgb_pixel, rgb_pixel)
	    = default;
};

struct greyscale_pixel {
	std::uint8_t value_;

	constexpr std::uint8_t value() const noexcept { return value_; }

	constexpr std::uint8_t& operator[](std::size_t) noexcept { return value_; }
	constexpr const std::uint8_t& operator[](std::size_t) const noexcept {
		return value_;
	}

	constexpr std::uint8_t& red() noexcept { return value_; }
	constexpr const std::uint8_t& red() const noexcept { return value_; }
	constexpr std::uint8_t& green() noexcept { return value_; }
	constexpr const std::uint8_t& green() const noexcept { return value_; }
	constexpr std::uint8_t& blue() noexcept { return value_; }
	constexpr const std::uint8_t& blue() const noexcept { return value_; }

	static auto& header(std::ostream& os, std::ptrdiff_t width,
	                    std::ptrdiff_t height) noexcept {
		return os << "P5" << ' ' << width << ' ' << height << " 255\n";
	}
	static std::filesystem::path extension() { return ".pgm"; }
	friend std::ostream& operator<<(std::ostream& os,
	                                const greyscale_pixel& px) {
		return os.put(static_cast<char>(px.value_));
	}
	constexpr static bool is_binary_writable = true;

	constexpr operator rgb_pixel() const noexcept {
		return {red(), green(), blue()};
	}
	constexpr operator rgba_pixel() const noexcept {
		return {red(), green(), blue(), 255};
	}
	constexpr friend std::strong_ordering operator<=>(greyscale_pixel,
	                                                  greyscale_pixel)
	    = default;
};
// Reads 6-character hexadecimal string into color
constexpr auto color_from_hex(std::string_view hex) -> rgb_pixel {
	std::uint8_t r{}, g{}, b{};
	uint_fast32_t c = kblib::parse_integer<uint_fast32_t>(hex, 16);
	r = (c & 0xFF0000u) >> 16u;
	g = (c & 0x00FF00u) >> 8u;
	b = (c & 0x0000FFu);
	return pnm::rgb_pixel{r, g, b};
}
inline auto color_to_hex(pnm::rgb_pixel color) -> std::string {
	constexpr static char hex[] = "0123456789abcdef";
	return {hex[color.red() / 16u],   hex[color.red() % 16u],
	        hex[color.green() / 16u], hex[color.green() % 16u],
	        hex[color.blue() / 16u],  hex[color.blue() % 16u]};
}

/**
 * @brief PNM (P6) image handler.
 *
 */
template <typename pixel>
class image {
 public:
	using iterator = typename std::vector<pixel>::iterator;
	using const_iterator = typename std::vector<pixel>::const_iterator;

	constexpr image() = default;
	image(std::ptrdiff_t width, std::ptrdiff_t height, pixel value)
	    : width_{width}
	    , data(kblib::to_unsigned(width * height), value) {
		assert(width >= 0 and height >= 0);
	}
	image(std::ptrdiff_t width, std::ptrdiff_t height)
	    : width_{width}
	    , data(kblib::to_unsigned(width * height)) {
		assert(width >= 0 and height >= 0);
	}
	image(std::ptrdiff_t width, std::ptrdiff_t height,
	      std::initializer_list<pixel> contents)
	    : width_(width)
	    , data(contents) {
		assert(width >= 0 and height >= 0);
		assert(std::cmp_equal(contents.size(), width * height));
	}

	constexpr void reshape(std::ptrdiff_t w, std::ptrdiff_t h) {
		if (w < 0 or h < 0) {
			throw std::invalid_argument{
			    "negative dimension specified for pnm::image::reshape"};
		}
		width_ = w;
		data.resize(w * h);
	}

#if __cpp_multidimensional_subscript >= 202211L
	constexpr pixel& operator[](std::ptrdiff_t x, std::ptrdiff_t y) noexcept {
		return data[index_linear<false>(x, y)];
	}
	constexpr const pixel& operator[](std::ptrdiff_t x,
	                                  std::ptrdiff_t y) const noexcept {
		return data[index_linear<false>(x, y)];
	}
#endif
	constexpr pixel& at(std::ptrdiff_t x, std::ptrdiff_t y) {
		return data[index_linear<true>(x, y)];
	}
	constexpr const pixel& at(std::ptrdiff_t x, std::ptrdiff_t y) const {
		return data[index_linear<true>(x, y)];
	}
	constexpr pixel& at(std::ptrdiff_t i) { return data.at(i); }
	constexpr const pixel& at(std::ptrdiff_t i) const { return data.at(i); }

	constexpr void assign(std::initializer_list<pixel> il) {
		assert(il.size() == data.size());
		data = il;
	}

	constexpr std::ptrdiff_t height() const noexcept {
		if (data.size() == 0) {
			return 0;
		} else {
			return kblib::to_signed(data.size()) / width_;
		}
	}
	constexpr std::ptrdiff_t width() const noexcept { return width_; }
	constexpr std::size_t size() const noexcept { return data.size(); }
	constexpr bool empty() const noexcept { return size() == 0; }

	constexpr iterator begin() { return data.begin(); }
	constexpr const_iterator begin() const { return data.begin(); }
	constexpr const_iterator cbegin() const { return data.begin(); }

	constexpr iterator end() { return data.end(); }
	constexpr const_iterator end() const { return data.end(); }
	constexpr const_iterator cend() const { return data.end(); }

	void write(std::ostream& os) const {
		pixel::header(os, width_, height());
		if constexpr (pixel::is_binary_writable) {
			os.write(reinterpret_cast<const char*>(data.data()),
			         data.size() * sizeof(pixel));
		} else {
			for (const auto& px : data) {
				os << px;
			}
		}
	}
	void write(std::ostream& os, std::ptrdiff_t scale_w,
	           std::ptrdiff_t scale_h) const {
		pixel::header(os, width_ * scale_w, height() * scale_h);
		for (const auto y : kblib::range(height())) {
			for (const auto _ : kblib::range(scale_h)) {
				for (const auto x : kblib::range(width_)) {
					for (const auto _ : kblib::range(scale_w)) {
						os << at(x, y);
					}
				}
			}
		}
	}

	friend std::ostream& operator<<(std::ostream& os, const image& img) {
		img.write(os);
		return os;
	}

	std::strong_ordering operator<=>(const image& other) const = default;

	template <bool safe>
	std::size_t index_linear(std::ptrdiff_t x, std::ptrdiff_t y) const
	    noexcept(not safe) {
		std::size_t r = static_cast<std::size_t>(y * width_ + x);
		if constexpr (safe) {
			if (x < 0 or y < 0 or x > width_ or r > data.size()) {
				throw std::out_of_range{kblib::concat("position (", x, ',', y,
				                                      ") out of range (", width_,
				                                      ',', height(), ")")};
			}
		}
		return r;
	}

 private:
	std::ptrdiff_t width_{};
	std::vector<pixel> data;
};

inline auto errno_to_message() -> std::string {
	return std::system_category().message(errno);
}

inline auto errno_to_error_code() noexcept -> std::error_code {
	return std::error_code(errno, std::system_category());
}

inline auto find_in_path(const std::filesystem::path& name)
    -> std::tuple<std::filesystem::path, std::error_code> {
	auto PATH = std::string_view(std::getenv("PATH"));

	for (const auto& p : kblib::split_dsv(PATH, ':')) {
		auto t = p / name;
		if (std::filesystem::exists(t)) {
			return {t, std::error_code{}};
		}
	}
	return {{}, std::make_error_code(std::errc::no_such_file_or_directory)};
}

template <typename pixel>
std::error_code finalize_to_png(const image<pixel>& img,
                                const std::filesystem::path& dest,
                                const std::filesystem::path& format,
                                std::string comments) {
	using namespace std::literals;
	auto pipename
	    = std::filesystem::temp_directory_path()
	      / kblib::concat(
	          dest.filename().native(), '-',
	          kblib::to_string(
	              std::chrono::system_clock::now().time_since_epoch().count(),
	              16),
	          pixel::extension().native());
	auto status = mkfifo(pipename.c_str(), S_IRUSR | S_IWUSR);
	if (status) {
		return errno_to_error_code();
	}
	auto _ = kblib::defer([&] { std::filesystem::remove(pipename); });

	if (auto pid = fork(); pid > 0) {
		// parent
		std::ofstream output(pipename);
		output << img;
	} else if (pid == 0) {
		// child
		auto d = [&] {
			if (format.empty()) {
				if (dest.extension().empty()) {
					return "png:" + dest.native();
				} else {
					return dest.native();
				}
			} else {
				return kblib::concat(format.native(), ':', dest.native());
			}
		}();
		auto p = pipename.native();
		char convert[] = "convert";
		char comment_arg[] = "-comment";
		char* cargs[] = {convert,  comment_arg, comments.data(),
		                 p.data(), d.data(),    nullptr};

		auto [convert_path, ec] = find_in_path("convert");
		if (ec) {
			std::cerr << "Could not find 'convert'\n";
			return ec;
		}

#if 0
		std::cerr << kblib::concat("Calling:\n\t",
		                           kblib::quoted(convert_path.native()), ' ',
		                           kblib::quoted(std::string_view(comment_arg)),
		                           ' ', kblib::quoted(comments), ' ',
		                           kblib::quoted(p), ' ', kblib::quoted(d), '\n');
#endif

		execv(convert_path.c_str(), cargs);
		{
			std::cerr << "Exec failure: " << errno_to_message() << '\n';
			std::ifstream in(pipename);
			while (in) {
				in.ignore(kblib::max);
			}
			in.close();
			abort();
		}
	} else {
		// failed to fork()
		std::cerr << "fork failure: " << errno_to_message() << '\n';
		return errno_to_error_code();
	}
	// success
	return {};
}

} // namespace pnm

inline std::error_code extract_comment_from_file(const char* filename,
                                                 std::string& out) {
	using namespace std::literals;
	auto pipename
	    = std::filesystem::temp_directory_path()
	      / kblib::concat(
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

#endif // IMAGE_HPP
