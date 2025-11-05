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

////////////////////////////////////////////////////////////////////////////////

// decription for sections inside an array
struct range
{
    size_t begin;
    size_t end;
};
#include "container.h"
using ranges = cvector<range>;

////////////////////////////////////////////////////////////////////////////////

// description of lines of a text file
struct LineInfo { UINT number; Yast text; };
using LineInfos = cvector<LineInfo>;

////////////////////////////////////////////////////////////////////////////////

// supported text encodings
enum TextEncoding
{
    TE_UNKNOWN,
    TE_BINARY,
    TE_ANSI,
    TE_UTF8,
    TE_UTF8_BOM,
    TE_UTF16_LE,
    TE_UTF16_LE_BOM
};
TextEncoding guess_encoding(const BYTE* data, size_t size, bool prefer_utf8);

////////////////////////////////////////////////////////////////////////////////

// wildcard matching
bool wild_match(PCWSTR tame, PCWSTR wild);

////////////////////////////////////////////////////////////////////////////////
