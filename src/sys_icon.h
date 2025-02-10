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

struct SysIconIdx
{
    static const DWORD idx_flags = (
        SHGFI_USEFILEATTRIBUTES |
        SHGFI_SYSICONINDEX |
        SHGFI_SMALLICON
        );

    static HIMAGELIST img_list()
    {
        SHFILEINFO sfi;
        return reinterpret_cast<HIMAGELIST>(
            SHGetFileInfo(
                L"nix",
                FILE_ATTRIBUTE_DIRECTORY,
                &sfi,
                sizeof(sfi),
                idx_flags
                )
            );
    }

    static int dir()
    {
        SHFILEINFO sfi;
        SHGetFileInfo(
            L"nix",
            FILE_ATTRIBUTE_DIRECTORY,
            &sfi,
            sizeof(sfi),
            idx_flags
            );
        return sfi.iIcon;
    }

    static int file(PCWSTR filename)
    {
        SHFILEINFO sfi;
        SHGetFileInfo(
            filename,
            0,
            &sfi,
            sizeof(sfi),
            idx_flags
            );
        return sfi.iIcon;
    }
};
