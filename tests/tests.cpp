/*
 * tests.cpp - test code core.
 *
 * Copyright (C) 2010 Adam Nielsen <malvineous@shikadi.net>
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

#define BOOST_TEST_MODULE libgamearchive
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/algorithm/string.hpp> // for case-insensitive string compare
#include <iostream>
#include <iomanip>

#include <camoto/gamearchive/manager.hpp>
#include <camoto/debug.hpp> // for ANSI colours
#include <camoto/util.hpp>  // createString
#include "tests.hpp"

void default_sample::printNice(boost::test_tools::predicate_result& res,
	const std::string& s, const std::string& diff)
{
	const char *c = CLR_YELLOW;
	res.message() << c;
	for (int i = 0; i < s.length(); i++) {
		if ((i > 0) && (i % 16 == 0)) {
			res.message() << CLR_NORM << "\n" << std::setfill('0') << std::setw(3)
				<< std::hex << i << ": " << c;
		}
		if ((i >= diff.length()) || (s[i] != diff[i])) {
			if (c != CLR_MAG) {
				c = CLR_MAG;
				res.message() << CLR_MAG;
			}
		} else {
			if (c != CLR_YELLOW) {
				c = CLR_YELLOW;
				res.message() << CLR_YELLOW;
			}
		}
		if ((s[i] < 32) || (s[i] == 127)) {
			res.message() << "\\x" << std::setfill('0') << std::setw(2)
				<< std::hex << (int)((uint8_t)s[i]);
		} else {
			res.message() << s[i];
		}
	}
	return;
}

void default_sample::print_wrong(boost::test_tools::predicate_result& res,
	const std::string& strExpected, const std::string& strResult)
{
	res.message() << "\nExp: ";
	this->printNice(res, strExpected, strResult);
	res.message() << CLR_NORM "\n" << "Got: ";
	this->printNice(res, strResult, strExpected);
	res.message() << CLR_NORM;

	return;
}

boost::test_tools::predicate_result default_sample::is_equal(
	const std::string& strExpected, const std::string& strCheck)
{
	if (strExpected.compare(strCheck)) {
		boost::test_tools::predicate_result res(false);
		this->print_wrong(res, strExpected, strCheck);
		return res;
	}

	return true;
}

void stringStreamTruncate(std::stringstream *ss, int len)
{
	if (len < ss->str().length()) {
		// Shrinking
		std::string orig = ss->str();
		ss->clear(); // reset state, leave string alone
		ss->str(orig.substr(0, len)); // set new string
	} else {
		// Enlarging
		std::streamsize pos;
		// Work around C++ stringstream bug that returns invalid offset when empty.
		// https://issues.apache.org/jira/browse/STDCXX-332
		if (!ss->str().empty()) {
			ss->seekp(0, std::ios::end);
			pos = ss->tellp();
			assert(pos > 0);
		} else {
			pos = 0;
		}

		*ss << std::string(len - pos, '\0');
		assert(ss->tellp() == len);
	}
	return;
}

void applyFilter(camoto::iostream_sptr *ppStream,
	camoto::gamearchive::Archive::EntryPtr id)
	throw (std::ios::failure)
{
	if (!id->filter.empty()) {
		// The file needs to be filtered first
		camoto::gamearchive::ManagerPtr pManager(camoto::gamearchive::getManager());
		camoto::gamearchive::FilterTypePtr pFilterType(
			pManager->getFilterTypeByCode(id->filter));
		if (!pFilterType) {
			throw std::ios::failure(createString(
				"could not find filter \"" << id->filter << "\""
			));
		}
		// TODO: use boost::bind to find the arch's truncate function
		*ppStream = pFilterType->apply(*ppStream, NULL);
	}
	return;
}
