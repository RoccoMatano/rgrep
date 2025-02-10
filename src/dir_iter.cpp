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

#include "pch.h"
#include "dir_iter.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DirectoryIterator::SingleDirIterator::SingleDirIterator(
    SingleDirIterator* parent,
    const Yast& pattern
    ) :
    m_path_prefix(pattern.str(), pattern.length() - 1),
    m_parent(parent),
    m_done_first(false)
{
    m_handle = FindFirstFileEx(
        pattern,
        FindExInfoBasic,
        &m_find_data,
        FindExSearchNameMatch,
        nullptr,
        // ignore pre Win7 systems and use
        FIND_FIRST_EX_LARGE_FETCH
        );
}

////////////////////////////////////////////////////////////////////////////////

bool DirectoryIterator::SingleDirIterator::next()
{
    bool found;
    if (!m_done_first)
    {
        m_done_first = true;
        found = (m_handle != INVALID_HANDLE_VALUE);
    }
    else
    {
        found = (FindNextFile(m_handle, &m_find_data) != 0);
    }

    while (found && is_dots())
    {
        found = (FindNextFile(m_handle, &m_find_data) != 0);
    }
    return found;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DirectoryIterator::DirectoryIterator(const Yast& dir_name) :
    m_dir_queue(nullptr),
    m_prefix_len(0),
    m_done_first(false)
{
    TRACE("dir iter: '%S'\n", dir_name.str());
    if (PathIsDirectory(dir_name))
    {
        PCWSTR add = L"*";
        m_prefix_len = dir_name.length();
        if (dir_name.str()[m_prefix_len - 1] != L'\\')
        {
            add = L"\\*";
            m_prefix_len++;
        }
        go_sub(dir_name + add);
    }
}

////////////////////////////////////////////////////////////////////////////////

DirectoryIterator::~DirectoryIterator()
{
    // have to go to top in order to release all memory
    while (m_dir_queue != nullptr)
    {
        go_up();
    }
}

////////////////////////////////////////////////////////////////////////////////

bool DirectoryIterator::next(Yast &path, bool& is_dir, bool go_down)
{
    if (m_dir_queue == nullptr)
    {
        return false;
    }

    if (!m_done_first)
    {
        m_done_first = true;
    }
    else if (go_down && m_dir_queue->is_dir())
    {
        go_sub(m_dir_queue->get_path() + L"\\*");
    }

    while (!m_dir_queue->next())
    {
        go_up();
        if (!m_dir_queue)
        {
            return false;
        }
    }

    path = m_dir_queue->get_path();
    TRACE("dir iter found: '%S'\n", path.str());
    is_dir = m_dir_queue->is_dir();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
