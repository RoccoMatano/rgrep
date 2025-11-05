////////////////////////////////////////////////////////////////////////////////
//
// This file is part of rgrep.
// rgrep is based on PCRE2 (see pcre2_16\LICENCE).
//
// Copyright 2018-2025 Rocco Matano
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "yast.h"
#include "rgrep_util.h"
#include <memory>

class rrx
{
public:

    // Flags that influence compilation and later matching.

    // Remove the special meaning from the characters in a given regex.
    static const UINT LITERAL       = 1 << 0;

    // Do caseless matching.
    static const UINT IGNORE_CASE   = 1 << 1;

    // Only match at word boudaries.
    static const UINT WHOLE_WORDS   = 1 << 2;

    // '.' matches anything including line end markers.
    static const UINT DOT_ALL       = 1 << 3;

    // Let '^' and '$' match at any beginning or end of a line in the
    // subject (not just at subject beginning and end).
    static const UINT MULTI_LINE    = 1 << 4;

    using ptr = std::shared_ptr<rrx>;

    //
    // Constructs a new rrx by compiling the given regular expression.
    // If the compilation fails, nullptr is returned.
    //
    static ptr compile(const Yast& regex, UINT flags = 0);

    //
    // Returns the first position at or behind offset, where this
    // pattern matches in a given string.
    //
    bool search(range& found, const Yast& subject, size_t offset = 0) const;

    //
    // Returns all the positions where this pattern matches in a given string.
    //
    ranges findall(const Yast& subject) const
    {
        ranges result;
        range match;
        for (size_t pos = 0; search(match, subject, pos); pos = match.end)
        {
            result.push_back(match);
        }
        return result;
    }


    //
    // Replaces found matches with 'replacement'.
    //
    Yast replace(const Yast& subject, const Yast& replacement) const;

    ~rrx();

private:
    rrx();
    rrx(const rrx&) = delete;
    rrx& operator=(const rrx&) = delete;

    class pimpl;
    pimpl *m_pimpl;
};
