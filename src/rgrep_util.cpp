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
#include "rgrep_util.h"

////////////////////////////////////////////////////////////////////////////////

static TextEncoding check_zeros(const BYTE* data, size_t size)
{
    const uint16_t* p16 = reinterpret_cast<const uint16_t*>(data);
    const uint16_t* const end = p16 + (size / sizeof(uint16_t));

    // scan buffer for zeros
    int z8 = 0;
    int z16 = 0;
    while (p16 < end)
    {
        uint16_t test = *p16++;
        if (test == 0)
        {
            ++z16;
            if (z16 > 2)   // arbitrary value
            {
                return TE_BINARY;
            }
        }
        if ((test & 0xff) == 0)
        {
            ++z8;
        }
        if ((test >> 8) == 0)
        {
            ++z8;
        }
    }
    if (data[0] == 0xff && data[1] == 0xfe)
    {
        return TE_UTF16_LE_BOM;
    }
    if ((z8 > 3) && ((size & 1) == 0)) // arbitrary value
    {
        return TE_UTF16_LE;
    }
    return TE_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////

static TextEncoding check_bom(const BYTE* data, size_t size)
{
    if (data[0] == 0xff && data[1] == 0xfe)
    {
        return TE_UTF16_LE_BOM;
    }
    if (size < 3)
    {
        return TE_ANSI;
    }
    if (data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf)
    {
        return TE_UTF8_BOM;
    }
    return TE_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////

TextEncoding guess_encoding(const BYTE* data, size_t size, bool prefer_utf8)
{
    if (size < 2)
    {
        return TE_ANSI;
    }
    TextEncoding e = check_zeros(data, size);
    if (e != TE_UNKNOWN)
    {
        return e;
    }
    e = check_bom(data, size);
    if (e != TE_UNKNOWN)
    {
        return e;
    }
    if (!prefer_utf8)
    {
        return TE_ANSI;
    }
    const int res = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        p2p<PCSTR>(data),
        static_cast<int>(size),
        nullptr,
        0
        );
    return (res == 0) ? TE_ANSI : TE_UTF8;
}

////////////////////////////////////////////////////////////////////////////////

bool wild_match(PCWSTR tame, PCWSTR wild)
{
    PCWSTR wm_tame = nullptr;
    PCWSTR wm_wild = nullptr;

    for (;;)
    {
        if (*wild == '*')
        {
            while (*(++wild) == '*');
            if (!*wild)
            {
                return true;
            }
            if (*wild != '?')
            {
                while (*tame != *wild)
                {
                    if (!(*(++tame)))
                    {
                        return false;
                    }
                }
            }
            wm_wild = wild;
            wm_tame = tame;
        }
        else if (*tame != *wild && *wild != '?')
        {
            if (wm_wild)
            {
                if (wild != wm_wild)
                {
                    wild = wm_wild;
                    if (*tame != *wild)
                    {
                        tame = ++wm_tame;
                        continue;
                    }
                    else
                    {
                        wild++;
                    }
                }
                if (*tame)
                {
                    tame++;
                    continue;
                }
            }
            return false;
        }

        tame++;
        wild++;
        if (*tame == 0)
        {
            while (*wild == '*')
            {
                wild++;
            }
            return (*wild == 0);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
