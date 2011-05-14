/**
 * @file   fmt-dat-bash.cpp
 * @brief  Implementation of Monster Bash .DAT file reader/writer.
 *
 * This file format is fully documented on the ModdingWiki:
 *   http://www.shikadi.net/moddingwiki/DAT_Format_%28Monster_Bash%29
 *
 * Copyright (C) 2010-2011 Adam Nielsen <malvineous@shikadi.net>
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

#include <boost/algorithm/string.hpp>

#include <camoto/iostream_helpers.hpp>
#include <camoto/util.hpp>

#include "fmt-dat-bash.hpp"

#define DAT_FIRST_FILE_OFFSET    0
#define DAT_MAX_FILENAME_LEN     30
#define DAT_FILENAME_FIELD_LEN   31

// Length of embedded-FAT entry
#define DAT_EFAT_ENTRY_LEN       37  // filename + offsets

#define DAT_FILETYPE_OFFSET(e)   ((e)->iOffset + 0)
#define DAT_FILESIZE_OFFSET(e)   ((e)->iOffset + 2)
#define DAT_FILENAME_OFFSET(e)   ((e)->iOffset + 4)
#define DAT_DECOMP_OFFSET(e)     ((e)->iOffset + 35)

namespace camoto {
namespace gamearchive {

DAT_BashType::DAT_BashType()
	throw ()
{
}

DAT_BashType::~DAT_BashType()
	throw ()
{
}

std::string DAT_BashType::getArchiveCode() const
	throw ()
{
	return "dat-bash";
}

std::string DAT_BashType::getFriendlyName() const
	throw ()
{
	return "Monster Bash DAT File";
}

// Get a list of the known file extensions for this format.
std::vector<std::string> DAT_BashType::getFileExtensions() const
	throw ()
{
	std::vector<std::string> vcExtensions;
	vcExtensions.push_back("dat");
	return vcExtensions;
}

std::vector<std::string> DAT_BashType::getGameList() const
	throw ()
{
	std::vector<std::string> vcGames;
	vcGames.push_back("Monster Bash");
	return vcGames;
}

E_CERTAINTY DAT_BashType::isInstance(iostream_sptr psArchive) const
	throw (std::ios::failure)
{
	psArchive->seekg(0, std::ios::end);
	io::stream_offset lenArchive = psArchive->tellg();
	// TESTED BY: fmt_dat_bash_isinstance_c02
	//if (lenArchive < DAT_FAT_OFFSET) return EC_DEFINITELY_NO; // too short

	psArchive->seekg(0, std::ios::beg);

	// Check each FAT entry
	char fn[DAT_FILENAME_FIELD_LEN];
	io::stream_offset pos = 0;
	uint16_t type, lenEntry;
	while (pos < lenArchive) {
		psArchive
			>> u16le(type)
			>> u16le(lenEntry);
		/*switch (type) {
			case 0: //?
			case 1: //?
			case 3:
			case 4:
			case 5:
			case 32:
			case 64:
				break;
			default:
				std::cout << "Unknown filetype number " << type << std::endl;
				return EC_DEFINITELY_NO;
		}*/
		psArchive->read(fn, DAT_FILENAME_FIELD_LEN);
		// Make sure there aren't any invalid characters in the filename
		for (int j = 0; j < DAT_MAX_FILENAME_LEN; j++) {
			if (!fn[j]) break; // stop on terminating null

			// Fail on control characters in the filename
			if (fn[j] < 32) return EC_DEFINITELY_NO; // TESTED BY: fmt_dat_bash_isinstance_c01
		}

		pos += lenEntry + DAT_EFAT_ENTRY_LEN;

		// If a file entry points past the end of the archive then it's an invalid
		// format.
		// TESTED BY: fmt_dat_bash_isinstance_c03
		if (pos > lenArchive) return EC_DEFINITELY_NO;

		psArchive->seekg(pos, std::ios::beg);
	}

	// If we've made it this far, this is almost certainly a DAT file.

	// TESTED BY: fmt_dat_bash_isinstance_c00, c02
	return EC_DEFINITELY_YES;
}

ArchivePtr DAT_BashType::newArchive(iostream_sptr psArchive, SuppData& suppData) const
	throw (std::ios::failure)
{
	return ArchivePtr(new DAT_BashArchive(psArchive));
}

// Preconditions: isInstance() has returned > EC_DEFINITELY_NO
ArchivePtr DAT_BashType::open(iostream_sptr psArchive, SuppData& suppData) const
	throw (std::ios::failure)
{
	return ArchivePtr(new DAT_BashArchive(psArchive));
}

SuppFilenames DAT_BashType::getRequiredSupps(const std::string& filenameArchive) const
	throw ()
{
	// No supplemental types/empty list
	return SuppFilenames();
}


DAT_BashArchive::DAT_BashArchive(iostream_sptr psArchive)
	throw (std::ios::failure) :
		FATArchive(psArchive, DAT_FIRST_FILE_OFFSET, DAT_MAX_FILENAME_LEN)
{
	this->psArchive->seekg(0, std::ios::end);
	io::stream_offset lenArchive = this->psArchive->tellg();

	this->psArchive->seekg(0, std::ios::beg);

	io::stream_offset pos = 0;
	uint16_t type;
	int numFiles = 0;
	while (pos < lenArchive) {
		FATEntry *fatEntry = new FATEntry();
		EntryPtr ep(fatEntry);

		fatEntry->iIndex = numFiles;
		fatEntry->iOffset = pos;
		fatEntry->lenHeader = DAT_EFAT_ENTRY_LEN;
		fatEntry->fAttr = 0;
		fatEntry->bValid = true;

		// Read the data in from the FAT entry in the file
		this->psArchive
			>> u16le(type)
			>> u16le(fatEntry->iSize)
			>> nullPadded(fatEntry->strName, DAT_FILENAME_FIELD_LEN)
			>> u16le(fatEntry->iPrefilteredSize);

		if (fatEntry->iPrefilteredSize) {
			fatEntry->fAttr |= EA_COMPRESSED;
			fatEntry->filter = "lzw-bash"; // decompression algorithm
		}

		switch (type) {
			case 3:
				fatEntry->type = "image/bash-tiles-bg";
				break;
			case 4:
				fatEntry->type = "image/bash-tiles-fg";
				break;
			case 5:
				fatEntry->type = "image/bash-tiles-bonus";
				break;
			case 64:
				fatEntry->type = "image/bash-sprite";
				break;
			case 32:
				fatEntry->type = FILETYPE_GENERIC;
				break;
			default:
				fatEntry->type = createString("unknown/bash-" << type);
				break;
		}
		this->vcFAT.push_back(ep);

		this->psArchive->seekg(fatEntry->iSize, std::ios::cur);
		pos += DAT_EFAT_ENTRY_LEN + fatEntry->iSize;
		numFiles++;
	}

}

DAT_BashArchive::~DAT_BashArchive()
	throw ()
{
}

int DAT_BashArchive::getSupportedAttributes() const
	throw ()
{
	return EA_COMPRESSED;
}

void DAT_BashArchive::updateFileName(const FATEntry *pid, const std::string& strNewName)
	throw (std::ios::failure)
{
	// TESTED BY: fmt_dat_bash_rename
	assert(strNewName.length() <= DAT_MAX_FILENAME_LEN);
	this->psArchive->seekp(DAT_FILENAME_OFFSET(pid));
	this->psArchive << nullPadded(strNewName, DAT_FILENAME_FIELD_LEN);
	return;
}

void DAT_BashArchive::updateFileOffset(const FATEntry *pid,
	std::streamsize offDelta
)
	throw (std::ios::failure)
{
	return;
}

void DAT_BashArchive::updateFileSize(const FATEntry *pid,
	std::streamsize sizeDelta
)
	throw (std::ios::failure)
{
	// TESTED BY: fmt_dat_bash_insert*
	// TESTED BY: fmt_dat_bash_resize*
	this->psArchive->seekp(DAT_FILESIZE_OFFSET(pid));
	this->psArchive << u16le(pid->iSize);

	// Write out the decompressed size too
	this->psArchive->seekp(DAT_FILENAME_FIELD_LEN, std::ios::cur);
	if (pid->fAttr & EA_COMPRESSED) {
		this->psArchive << u16le(pid->iPrefilteredSize);
	} else {
		this->psArchive << u16le(0);
	}
	return;
}

FATArchive::FATEntry *DAT_BashArchive::preInsertFile(
	const FATEntry *idBeforeThis, FATEntry *pNewEntry
)
	throw (std::ios::failure)
{
	// TESTED BY: fmt_dat_bash_insert*
	assert(pNewEntry->strName.length() <= DAT_MAX_FILENAME_LEN);

	// Set the format-specific variables
	pNewEntry->lenHeader = DAT_EFAT_ENTRY_LEN;

	// Because the new entry isn't in the vector yet we need to shift it manually
	//pNewEntry->iOffset += DAT_EFAT_ENTRY_LEN;

	this->psArchive->seekp(pNewEntry->iOffset);
	this->psArchive->insert(DAT_EFAT_ENTRY_LEN);
	boost::to_upper(pNewEntry->strName);

	if (pNewEntry->fAttr & EA_COMPRESSED) {
		pNewEntry->filter = "lzw-bash";
	}

	// Since we've inserted some data for the embedded header, we need to update
	// the other file offsets accordingly.  This call updates the offset of the
	// files, then calls updateFileOffset() on them, using the *new* offset, so
	// we need to do this after the insert() call above to make sure the extra
	// data has been inserted.  Then when updateFileOffset() writes data out it
	// will go into the correct spot.
	this->shiftFiles(NULL, pNewEntry->iOffset, pNewEntry->lenHeader, 0);

	return pNewEntry;
}

void DAT_BashArchive::postInsertFile(FATEntry *pNewEntry)
	throw (std::ios::failure)
{
	this->psArchive->seekp(pNewEntry->iOffset);

	int typeNum;
	if (pNewEntry->type.empty()) {
		typeNum = 32;
	} else if (pNewEntry->type.compare("image/bash-tiles-bg") == 0) {
		typeNum = 3;
	} else if (pNewEntry->type.compare("image/bash-tiles-fg") == 0) {
		typeNum = 4;
	} else if (pNewEntry->type.compare("image/bash-tiles-bonus") == 0) {
		typeNum = 5;
	} else if (pNewEntry->type.compare("image/bash-sprite") == 0) {
		typeNum = 64;
	} else if (pNewEntry->type.substr(0, 13).compare("unknown/bash-") == 0) {
		typeNum = strtod(pNewEntry->type.substr(14).c_str(), NULL);
	}

	uint16_t expandedSize;
	if (pNewEntry->fAttr & EA_COMPRESSED) {
		expandedSize = pNewEntry->iPrefilteredSize;
	} else {
		expandedSize = 0;
	}

	// Write out the entry
	this->psArchive
		<< u16le(typeNum)
		<< u16le(pNewEntry->iSize)
		<< nullPadded(pNewEntry->strName, DAT_FILENAME_FIELD_LEN)
		<< u16le(expandedSize)
	;
	return;
}

} // namespace gamearchive
} // namespace camoto
