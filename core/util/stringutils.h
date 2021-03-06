﻿/*
stringutils.h
Copyright (C) 2010-2015 Marc GILLERON
This file is part of the SnowfeetEngine project.
*/

#ifndef __HEADER_SN_STRINGUTILS__
#define __HEADER_SN_STRINGUTILS__

#include <string>
#include <vector>
#include <core/types.h>

namespace sn
{
// TODO stringutils: use std::to_string and make standard functions
// But MinGW doesn't seems to include it yet...

std::string SN_API toString(f32 x);
std::string SN_API toString(f64 x);
std::string SN_API toString(s32 x);
std::string SN_API toString(u32 x);
std::string SN_API toString(bool b);

std::string SN_API toStringBin(u32 x, u8 n=32);
std::string SN_API toStringAlnum(s32 x);

s32 SN_API toInt(std::string s);
void SN_API toUpper(std::string & str);
void SN_API toLower(std::string & str);

// convert a time given in seconds to a time string formated like "hh:mm:ss:ms"
std::string SN_API toTimeString(f32 s);

bool SN_API isPrintableChar(u8 c);

// TODO Move file related functions to core/system/file
std::string SN_API getFileNameWithoutExtension(std::string path);

std::string SN_API toString(const std::wstring & wstr);
std::wstring SN_API toWideString(const std::string & str);

/// \brief Gets the file extension within the given path, if any (the dot is included).
std::string SN_API getFileExtension(const std::string & str);

/// \brief Gets the file extension within the given path, if any (the dot is included).
std::wstring SN_API getFileExtension(const std::wstring & str);

// Returns the folder in which the given file is.
// If the path points to a folder, the parent folder is returned.
// If the path ends with a '/' or a '\', the function just removes it.
std::wstring SN_API getFileFolder(const std::wstring & path);
std::string SN_API getFileFolder(const std::string & path);

/// \brief Returns a substring without trailing whitespace.
std::string SN_API trimLeft(std::string str);

/// \brief Returns a substring without leading whitespace. 
std::string SN_API trimRight(std::string str);

/// \brief Returns a substring without surrounding whitespace.
std::string SN_API trim(std::string str);

/// \brief Splits the given string at sep and returns the list without sep
std::vector<std::string> SN_API split(const std::string & str, char sep);

/// \brief Tests if the given character is one of the end-of-line / newline characters
inline bool isEOL(char c) { return c == '\n' || c == '\r'; }

/// \brief Reads a whole file and puts its content into the given string.
/// \param filePath: path to the file
/// \param str: output string
/// \return true on success, false if the file couldn't be opened
bool SN_API readFile(const std::string & filePath, std::string & str);

/// \brief Concatenates strings with an optional separator between them.
/// \param stringList: strings container (STL compliant)
/// \param sep: separator
/// \return concatenated string
template <typename List_T>
std::string join(const List_T & stringList, const std::string sep = "")
{
    if (stringList.empty())
        return "";

    // Put first element
    auto it = stringList.begin();
    std::string res = *it;

    // While there are following elements
    for (++it; it != stringList.end(); ++it)
    {
        // Put a separator
        res += sep;
        // Concatenate the element
        const std::string & s = *it;
        res += s;
    }

    return res;
}

} // namespace sn

#endif // __HEADER_SN_STRINGUTILS__


