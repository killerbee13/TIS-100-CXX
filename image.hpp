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
#include <kblib/stringops.h>

#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace pnm {

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
		log_debug("image built from ", data.size(), " values");
		assert(std::cmp_equal(data.size(), width * height));
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
	constexpr bool blank() const noexcept {
		return std::ranges::none_of(data, [](pixel p) { return p != pixel{}; });
	}

	constexpr iterator begin() { return data.begin(); }
	constexpr const_iterator begin() const { return data.begin(); }
	constexpr const_iterator cbegin() const { return data.begin(); }

	constexpr iterator end() { return data.end(); }
	constexpr const_iterator end() const { return data.end(); }
	constexpr const_iterator cend() const { return data.end(); }

	bool operator==(const image& other) const {
		// to do this optimization we'd technically need to use
		// `__is_trivially_equality_comparable(pixel)`, but it exists only in
		// clang as of today, so we use a combo that is a good substitute for any
		// sane pixel type
		// see https://stackoverflow.com/q/78887411/3288954 and linked for context
		if constexpr (std::is_trivially_copyable_v<pixel>
		              && std::has_unique_object_representations_v<pixel>) {
			return width_ == other.width_ && data.size() == other.data.size()
			       && std::memcmp(data.data(), other.data.data(),
			                      data.size() * sizeof(pixel))
			              == 0;
		} else {
			return width_ == other.width_ && data == other.data;
		}
	}

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

} // namespace pnm

struct tis_pixel {
	// The ingame colors are: 000000, 464646, 9C9C9C, FDFDFD, C10B0B
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
	enum color : unsigned char {
		C_black,
		C_dark_grey,
		C_light_grey,
		C_white,
		C_red
	} val{};

	static constexpr color normalize(auto c) {
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
	constexpr tis_pixel(std::floating_point auto c) noexcept
	    : val(normalize(static_cast<std::underlying_type_t<color>>(c))) {}

	constexpr friend std::strong_ordering operator<=>(tis_pixel, tis_pixel)
	    = default;
};

struct image_t : pnm::image<tis_pixel> {
	using image::image;
	using enum tis_pixel::color;

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
			return;
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
				os << rkey_[etoi((*this)[x, y].val)];
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
				ret += rkey_[etoi((*this)[x, y].val)];
			}
			ret += '\n';
		}
		return ret;
	}

	constexpr std::string write_text(bool colored) const {
		return write_text(
		    {" ", "░", "▒", "█",
		     colored ? concat(escape_code(red), "▓", escape_code(reset_color))
		             : "#"});
	}
};

#endif // IMAGE_HPP
