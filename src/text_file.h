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

#include "rgrep_util.h"

class TextFile
{
public:

    TextFile() : m_size(0), m_encoding(TE_UNKNOWN)
    {
    }

    bool load(const Yast& path, bool prefer_utf8, bool include_binary);
    bool store(const Yast& path);
    LineInfos lines_from_ranges(const ranges& bounds);

    const Yast& get_path() const
    {
        return m_path;
    }

    const Yast& get_content()
    {
        return m_content;
    }

    void move_to_content(Yast& new_content)
    {
        m_content = std::move(new_content);
    }

    TextEncoding get_encoding()
    {
        return m_encoding;
    }

protected:

    BYTE* map_file(const Yast& path, size_t max_size);

    cvector<size_t> m_line_ends;
    Yast m_path;
    Yast m_content;
    size_t m_size;
    TextEncoding m_encoding;
};
