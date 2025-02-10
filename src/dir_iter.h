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

class DirectoryIterator
{
protected:

    struct SingleDirIterator
    {
        WIN32_FIND_DATAW m_find_data;
        HANDLE m_handle;
        Yast m_path_prefix;
        SingleDirIterator *m_parent;
        bool m_done_first;

        SingleDirIterator(SingleDirIterator* parent, const Yast& path);
        bool next();

        bool is_dots()
        {
            const WCHAR *const n = m_find_data.cFileName;
            return (
                (n[0] == L'.') &&
                ((n[1] == 0) || ((n[1] == L'.') && (n[2] == 0)))
                );
        }

        bool is_dir()
        {
            return !!(m_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        }

        Yast get_path()
        {
            PCWSTR cstr = m_find_data.cFileName;
            return m_path_prefix + cstr;
        }

        const WIN32_FIND_DATA* get_info()
        {
            return &m_find_data;
        }

        ~SingleDirIterator()
        {
            FindClose(m_handle);
        }
    };

    SingleDirIterator* m_dir_queue;
    UINT m_prefix_len;
    bool m_done_first;

    void go_sub(const Yast& dir_name)
    {
        TRACE("sub: '%S'\n", dir_name.str());
        m_dir_queue = new SingleDirIterator(m_dir_queue, dir_name);
    }

    void go_up()
    {
        SingleDirIterator *obsolete = m_dir_queue;
        m_dir_queue = m_dir_queue->m_parent;
        delete obsolete;
    }

public:
    DirectoryIterator(const Yast& dir_name);
    ~DirectoryIterator();
    bool next(Yast &path, bool& is_dir, bool go_down);

    const WIN32_FIND_DATA* get_info()
    {
        return m_dir_queue ? m_dir_queue->get_info() : nullptr;
    }

    UINT prefix_len()
    {
        return m_prefix_len;
    }

};

////////////////////////////////////////////////////////////////////////////////
