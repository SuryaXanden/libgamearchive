/**
 * @file   filter-zone66.hpp
 * @brief  Filter implementation for decompressing Zone 66 files.
 *
 * Copyright (C) 2010-2012 Adam Nielsen <malvineous@shikadi.net>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CAMOTO_FILTER_ZONE66_HPP_
#define _CAMOTO_FILTER_ZONE66_HPP_

#include <stack>
#include <camoto/stream.hpp>
#include <camoto/bitstream.hpp>
#include <camoto/gamearchive/filtertype.hpp>

namespace camoto {
namespace gamearchive {

class filter_z66_decompress: virtual public filter
{
	public:
		filter_z66_decompress();
		virtual ~filter_z66_decompress();

		int nextChar(const uint8_t **in, stream::len *lenIn, stream::len *r,
			uint8_t *out);

		virtual void reset();
		virtual void transform(uint8_t *out, stream::len *lenOut,
			const uint8_t *in, stream::len *lenIn);

	protected:
		bitstream data;
		int state;

		int code, curCode;

		std::stack<int> stack;
		int codeLength, curDicIndex, maxDicIndex;

		struct {
			int code, nextCode;
		} nodes[8192];
};

/// Zone 66 decompression filter
class Zone66FilterType: virtual public FilterType
{
	public:
		Zone66FilterType();
		virtual ~Zone66FilterType();

		virtual std::string getFilterCode() const;
		virtual std::string getFriendlyName() const;
		virtual std::vector<std::string> getGameList() const;
		virtual stream::inout_sptr apply(stream::inout_sptr target,
			stream::fn_truncate resize) const;
		virtual stream::input_sptr apply(stream::input_sptr target) const;
		virtual stream::output_sptr apply(stream::output_sptr target,
			stream::fn_truncate resize) const;
};

} // namespace gamearchive
} // namespace camoto

#endif // _CAMOTO_FILTER_ZONE66_HPP_
