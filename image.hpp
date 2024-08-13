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
#ifndef IMAGE_HPP
#define IMAGE_HPP
#include "logger.hpp"
#include "utils.hpp"

#include <kblib/convert.h>
#include <kblib/simple.h>
#include <kblib/stats.h>
#include <kblib/stringops.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

using kblib::concat, kblib::etoi, kblib::range, kblib::to_signed,
    kblib::to_unsigned;

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
	r = static_cast<std::uint8_t>((c & 0xFF0000u) >> 16u);
	g = static_cast<std::uint8_t>((c & 0x00FF00u) >> 8u);
	b = static_cast<std::uint8_t>(c & 0x0000FFu);
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
	    , data(to_unsigned(width * height), value) {
		assert(width >= 0 and height >= 0);
	}
	image(std::ptrdiff_t width, std::ptrdiff_t height)
	    : width_{width}
	    , data(to_unsigned(width * height)) {
		assert(width >= 0 and height >= 0);
	}
	image(std::ptrdiff_t width, [[maybe_unused]] std::ptrdiff_t height,
	      std::initializer_list<pixel> contents)
	    : width_(width)
	    , data(contents) {
		assert(width >= 0 and height >= 0);
		assert(std::cmp_equal(contents.size(), width * height));
	}
	image(std::ptrdiff_t width, [[maybe_unused]] std::ptrdiff_t height,
	      std::vector<pixel> contents)
	    : width_(width)
	    , data(std::move(contents)) {
		assert(width >= 0 and height >= 0);
		assert(std::cmp_equal(contents.size(), width * height));
	}

	// Not called resize to emphasize that image geometry is not preserved
	constexpr void reshape(std::ptrdiff_t w, std::ptrdiff_t h) {
		if (w < 0 or h < 0) {
			throw std::invalid_argument{
			    "negative dimension specified for pnm::image::reshape"};
		}
		width_ = w;
		data.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
	}
	constexpr void reshape(std::ptrdiff_t w, std::ptrdiff_t h, pixel p) {
		if (w < 0 or h < 0) {
			throw std::invalid_argument{
			    "negative dimension specified for pnm::image::reshape"};
		}
		width_ = w;
		data.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), p);
	}

	constexpr void fill(pixel p) { std::ranges::fill(data, p); }

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
	constexpr pixel& at(std::size_t i) { return data.at(i); }
	constexpr const pixel& at(std::size_t i) const { return data.at(i); }

	constexpr void assign(std::initializer_list<pixel> il) {
		assert(il.size() == data.size());
		data = il;
	}
	constexpr void assign(std::vector<pixel> il) {
		assert(il.size() == data.size());
		data = std::move(il);
	}

	constexpr std::ptrdiff_t height() const noexcept {
		if (data.size() == 0) {
			return 0;
		} else {
			return to_signed(data.size()) / width_;
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
		for (const auto y : range(height())) {
			for ([[maybe_unused]] const auto _ : range(scale_h)) {
				for (const auto x : range(width_)) {
					for ([[maybe_unused]] const auto _1 : range(scale_w)) {
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
				throw std::out_of_range{concat("position (", x, ',', y,
				                               ") out of range (", width_, ',',
				                               height(), ")")};
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
	      / concat(
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
				return concat(format.native(), ':', dest.native());
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

std::error_code extract_comment_from_file(const char* filename,
                                          std::string& out);

struct tis_pixel {
	enum color : unsigned char {
		C_black,
		C_dark_grey,
		C_light_grey,
		C_white,
		C_red
	} val{};
	static constexpr std::array<pnm::rgb_pixel, 5> colors{
	    pnm::color_from_hex("000000"), pnm::color_from_hex("464646"),
	    pnm::color_from_hex("9C9C9C"), pnm::color_from_hex("FDFDFD"),
	    pnm::color_from_hex("C10B0B"),
	    // The game uses floats, {
	    //    { 0, new Color(0f, 0f, 0f) },
	    //    { 1, new Color(0.273f, 0.273f, 0.273f) },
	    //    { 2, new Color(0.547f, 0.547f, 0.547f) },
	    //    { 3, new Color(0.808f, 0.808f, 0.808f) },
	    //    { 4, new Color(0.648f, 0.043f, 0.043f) }
	    // };
	    // Which I think convert to
	    // 000000, 464646, 8B8B8B, CECECE, AE0B0B
	    // but the above colors are picked from screenshots so the game must have
	    // a shader that darkens them
	};

	static color normalize(auto c) {
		if (c > C_red or c < 0) {
			return C_black;
		}
		return static_cast<color>(c);
	}

	tis_pixel() = default;
	constexpr tis_pixel(color c) noexcept
	    : val(normalize(c)) {}
	explicit(false) constexpr tis_pixel(std::integral auto c) noexcept
	    : val(normalize(c)) {}

	pnm::rgb_pixel as_rgb() const { return colors[val]; }
	constexpr operator pnm::rgb_pixel() const { return colors[val]; }
	constexpr operator pnm::rgba_pixel() const {
		return static_cast<pnm::rgba_pixel>(colors[val]);
	}

	constexpr std::uint8_t& operator[](std::size_t i) noexcept {
		return as_rgb()[i];
	}
	constexpr const std::uint8_t& operator[](std::size_t i) const noexcept {
		return as_rgb()[i];
	}

	constexpr std::uint8_t& red() noexcept { return as_rgb()[0]; }
	constexpr const std::uint8_t& red() const noexcept { return as_rgb()[0]; }
	constexpr std::uint8_t& green() noexcept { return as_rgb()[1]; }
	constexpr const std::uint8_t& green() const noexcept { return as_rgb()[1]; }
	constexpr std::uint8_t& blue() noexcept { return as_rgb()[2]; }
	constexpr const std::uint8_t& blue() const noexcept { return as_rgb()[2]; }

	// std::pow is not usable in constexpr, so this has to be precomputed
	static constexpr std::array<std::uint8_t, 5> values{0, 142, 205, 254, 122};
	constexpr std::uint8_t value() const noexcept { return values[val]; }

	// PNM doesn't have an indexed version so full RGB is the only option
	static auto& header(std::ostream& os, std::ptrdiff_t width,
	                    std::ptrdiff_t height) noexcept {
		return os << "P6" << ' ' << width << ' ' << height << " 255\n";
	}
	static std::filesystem::path extension() { return ".ppm"; }
	friend std::ostream& operator<<(std::ostream& os, const tis_pixel& px) {
		return os << px.as_rgb();
	}
	constexpr static bool is_binary_writable = false;
	constexpr friend std::strong_ordering operator<=>(tis_pixel, tis_pixel)
	    = default;
};

struct image_t : pnm::image<tis_pixel> {
	using image::image;
	using enum tis_pixel::color;
	constexpr image_t() = default;
	image_t(std::ptrdiff_t width, std::ptrdiff_t height, tis_pixel value)
	    : image(width, height, value) {}
	image_t(std::ptrdiff_t width, std::ptrdiff_t height, int value)
	    : image(width, height, value) {}
	image_t(std::ptrdiff_t width, std::ptrdiff_t height,
	        std::initializer_list<tis_pixel> contents)
	    : image(width, height, contents) {}
	image_t(std::ptrdiff_t width, std::ptrdiff_t height,
	        std::initializer_list<int> contents)
	    : image(width, height) {
		assert(contents.size() == size());
		std::copy(contents.begin(), contents.end(), begin());
	}
	image_t(std::initializer_list<std::u16string_view> image,
	        std::u16string_view key_ = key) {
		assign(image, key_);
	}
	// char16_t needed to have random access
	constexpr static std::u16string_view key = u" ░▒█#";
	// the reverse key needs to separate them instead
	constexpr static std::array<std::string_view, 5> rkey
	    = {" ", "░", "▒", "█", "#"};

	using image::assign;
	// It's a lot more convenient to use single characters for pixels instead of
	// using the enum values or comma-separated integers
	constexpr void assign(std::u16string_view image,
	                      std::u16string_view key_ = key) {
		assert(image.size() == size());
		auto it = begin();
		for (auto px : image) {
			if (px == key_[0]) {
				*it++ = {C_black};
			} else if (px == key_[1]) {
				*it++ = {C_dark_grey};
			} else if (px == key_[2]) {
				*it++ = {C_light_grey};
			} else if (px == key_[3]) {
				*it++ = {C_white};
			} else if (px == key_[4]) {
				*it++ = {C_red};
			}
		}
	}

	constexpr void assign(std::initializer_list<std::u16string_view> image,
	                      std::u16string_view key_ = key) {
		if (image.size() == 0) {
			reshape(0, 0);
		}
		std::size_t w{image.begin()[0].size()};
#if not RELEASE
		for (auto line : image) {
			assert(line.size() == w);
		}
#endif
		reshape(to_signed(w), to_signed(image.size()));
		auto it = begin();
		for (auto line : image) {
			for (auto px : line) {
				if (px == key_[0]) {
					*it++ = {C_black};
				} else if (px == key_[1]) {
					*it++ = {C_dark_grey};
				} else if (px == key_[2]) {
					*it++ = {C_light_grey};
				} else if (px == key_[3]) {
					*it++ = {C_white};
				} else if (px == key_[4]) {
					*it++ = {C_red};
				}
			}
		}
	}

	constexpr std::ostream& write_text(
	    std::ostream& os,
	    const std::array<std::string_view, 5>& rkey_ = rkey) const {
		for (const auto y : range(height())) {
			for (const auto x : range(width())) {
				os << rkey_[etoi(at(x, y).val)];
			}
			os << '\n';
		}
		return os;
	}

	constexpr std::string write_text(const std::array<std::string_view, 5>& rkey_
	                                 = rkey) const {
		std::string ret;
		for (const auto y : range(height())) {
			for (const auto x : range(width())) {
				ret += rkey_[etoi(at(x, y).val)];
			}
			ret += '\n';
		}
		return ret;
	}

	constexpr std::string write_text(bool colored) const {
		return write_text(
		    {" ", "░", "▒", "█",
		     colored ? concat(print_escape(red), "▓", print_escape(reset_color))
		             : "#"});
	}
};

#endif // IMAGE_HPP
