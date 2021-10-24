////////////////////////////////////////////////////////////////////////////////
//
// This file is part of rgrep.
// rgrep is based on PCRE2 (see pcre2_16\LICENCE).
//
// Copyright 2018-2021 Rocco Matano
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

#include "pch.h"
#include "text_file.h"

////////////////////////////////////////////////////////////////////////////////

BYTE* TextFile::map_file(const Yast& path, size_t max_size)
{
    HANDLE file = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        0
        );

    if (file == INVALID_HANDLE_VALUE)
    {
        return nullptr;
    }

    MEMORYSTATUSEX mstat = {sizeof(MEMORYSTATUSEX)};
    GlobalMemoryStatusEx(&mstat);
    const UINT threshold_percent = 20U;
    const UINT thresh_div = 100U / threshold_percent;
    const ULONGLONG threshold = mstat.ullAvailPhys / thresh_div;

    LARGE_INTEGER lsize;
    GetFileSizeEx(file, &lsize);
    ULONGLONG fsize = lsize.QuadPart;
    if (fsize > max_size || fsize > threshold)
    {
        CloseHandle(file);
        return nullptr;
    }

    HANDLE mapping = CreateFileMappingW(
        file,
        nullptr,
        PAGE_READONLY,
        0,
        0,
        nullptr
        );
    CloseHandle(file);
    if (mapping == nullptr)
    {
        return nullptr;
    }

    BYTE* pBase = static_cast<BYTE*>(
        MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0)
        );
    CloseHandle(mapping);
    if (pBase)
    {
        m_size = static_cast<size_t>(fsize);
    }
    return pBase;
}

////////////////////////////////////////////////////////////////////////////////

bool TextFile::load(const Yast& path, bool prefer_utf8, bool include_binary)
{
    m_line_ends.clear();
    m_content.clear();
    m_encoding = TE_UNKNOWN;
    m_size = 0;
    m_path = path;

    // Currently Yast is deliberately designed so that its size
    // cannot exceed 32 bits.
    const BYTE* const mapping = map_file(path, Yast::MAX_LEN);
    if (mapping == nullptr)
    {
        return false;
    }
    m_encoding = guess_encoding(mapping, m_size, prefer_utf8);
    if (m_encoding == TE_BINARY && !include_binary)
    {
        UnmapViewOfFile(mapping);
        return false;
    }

    PCSTR p_cnv = reinterpret_cast<PCSTR>(mapping);
    const UINT keep_utf16 = ~0u;
    const UINT bin_to_utf16 = keep_utf16 - 1;
    UINT c_size = static_cast<UINT>(m_size);
    UINT cp = CP_ACP;
    switch (m_encoding)
    {
        default:
        case TE_ANSI:
            break;

        case TE_BINARY:
            cp = bin_to_utf16;
            break;

        case TE_UTF8_BOM:
            c_size -= 3;
            p_cnv += 3;     // skip BOM
                            // fall through
        case TE_UTF8:
            cp = CP_UTF8;
            break;

        case TE_UTF16_LE_BOM:
            c_size -= 2;
            p_cnv += 2;     // skip BOM
                            // fall through
        case TE_UTF16_LE:
            cp = keep_utf16;
            break;
    }

    switch (cp)
    {
        case bin_to_utf16:
        {
            // store each byte as the corresponding code point
            m_content.clear(c_size);
            PWSTR dst = const_cast<PWSTR>(m_content.str());
            for (UINT u = 0; u < c_size; u++)
            {
                dst[u] = mapping[u];
            }
            break;
        }

        case keep_utf16:
        {
            // keep utf16 encoding that is already present
            PCWSTR p_utf16 = reinterpret_cast<PCWSTR>(p_cnv);
            m_content = Yast(p_utf16, c_size / sizeof(WCHAR));
            break;
        }

        default:
            // convert from cp to utf16
            m_content = Yast(c_size, p_cnv, cp);
            break;
    }

    UnmapViewOfFile(mapping);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

LineInfos TextFile::lines_from_ranges(const ranges& bounds)
{
    // Reserve one info entry for every range. Overlapping is rare.
    LineInfos result;
    result.reserve(bounds.size());
    auto rit = bounds.begin();
    if (rit == bounds.end())
    {
        return result;
    }

    if (m_encoding == TE_BINARY)
    {
        // No line ends available -> Report range begin and empty line.
        do
        {
            result.push_back(
                LineInfo { static_cast<UINT>(rit->begin), Yast() }
                );
        } while (++rit != bounds.end());
        return result;
    }

    // determine line ends, expecting an average line length of 32
    m_line_ends.reserve(m_content.length() / 32);
    PCWSTR it = m_content.str();
    PCWSTR const begin = it;
    PCWSTR const end = it + m_content.length();
    while(it < end)
    {
        if (*it == L'\r')
        {
            size_t pos = it++ - begin;
            if (*it == L'\n')
            {
                pos += 1;
            }
            m_line_ends.push_back(pos);
        }
        else if (*it == L'\n')
        {
            m_line_ends.push_back(it - begin);
        }
        ++it;
    }
    m_line_ends.push_back(m_content.length());


    auto lit = m_line_ends.begin();
    auto const first_line = lit;

    bool wait_for_range_start = true;
    while (lit != m_line_ends.end())
    {
        bool add_line = false;
        if (wait_for_range_start && (rit->begin <= *lit))
        {
            // This line contains a range start.
            add_line = true;
            wait_for_range_start = false;
        }
        else if (!wait_for_range_start && (rit->end > *lit))
        {
            // This is one of the lines that are completely inside of a range.
            add_line = true;
        }
        else if (!wait_for_range_start && (rit->end <= *lit))
        {
            // On this line a range ends.
            add_line = wait_for_range_start = true;
        }
        if (add_line)
        {
            const UINT line_no = static_cast<UINT>((lit - first_line) + 1);
            const UINT line_end = static_cast<UINT>(*lit);
            const UINT line_begin = (
                (lit == first_line) ?
                0 :
                static_cast<UINT>(*(lit - 1) + 1)
                );
            result.push_back(
                LineInfo { line_no, m_content.slice(line_begin, line_end) }
                );
            // We added the line. So we can - and have to - skip all ranges
            // that still end on this very line.
            while (line_end >= rit->end)
            {
                if (++rit == bounds.end())
                {
                    return result;
                }
            }
            // If the next interesting range - being now pointed to by rit -
            // starts behind the line we just added, we have to expect a
            // range start - no matter what the current value of
            // wait_for_range_start is.
            if (rit->begin > *lit)
            {
                wait_for_range_start = true;
            }
        }
        ++lit;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////

bool TextFile::store(const Yast& path)
{
    HANDLE file = CreateFile(
        path,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
        );
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD num_written;

    bool r = true;
    const UINT keep_utf16 = ~0U;
    UINT cp = CP_ACP;
    switch (m_encoding)
    {
        default:
        case TE_BINARY:
        case TE_ANSI:
            break;

        case TE_UTF8_BOM:
            r = r && WriteFile(file, "\xef\xbb\xbf", 3, &num_written, nullptr);
            // fall through

        case TE_UTF8:
            cp = CP_UTF8;
            break;

        case TE_UTF16_LE_BOM:
            r = r && WriteFile(file, "\xfe\xff", 2, &num_written, nullptr);
            // fall through

        case TE_UTF16_LE:
            cp = keep_utf16;
            break;
    }

    PCSTR p_data = reinterpret_cast<PCSTR>(m_content.str());
    UINT len = m_content.byte_length();
    CharFromW cvt(cp, nullptr);
    if (cp != keep_utf16)
    {
        cvt = m_content.str();
        len = cvt.length();
        p_data = cvt.str();
    }

    r = r && WriteFile(file, p_data, len, &num_written, nullptr);
    return (CloseHandle(file) && r);
}

////////////////////////////////////////////////////////////////////////////////