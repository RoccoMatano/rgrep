////////////////////////////////////////////////////////////////////////////////
//
// This file is part of rgrep.
// rgrep is based on PCRE2 (see pcre2_16\LICENCE).
//
// Copyright 2018-2022 Rocco Matano
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
#include "search_thread.h"
#include "dir_iter.h"

////////////////////////////////////////////////////////////////////////////////

SearchThread::SearchThread() :
    m_lock(SRWLOCK_INIT),
    m_running(0),
    m_canceled(0)
{
}

////////////////////////////////////////////////////////////////////////////////

SearchThread::~SearchThread()
{
}

////////////////////////////////////////////////////////////////////////////////

bool SearchThread::start(SearchParams& params)
{
    if (!m_running && !m_canceled)
    {
        m_params = params;

        DWORD tid;
        HANDLE hThread = CreateThread(
            nullptr,
            0,
            thread_proc,
            this,
            0,
            &tid
            );
        if (hThread)
        {
            InterlockedExchange(&m_running, true);
            CloseHandle(hThread);
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void SearchThread::cancel()
{
    InterlockedExchange(&m_canceled, true);
}

////////////////////////////////////////////////////////////////////////////////

bool SearchThread::is_running()
{
    return m_running != 0;
}

////////////////////////////////////////////////////////////////////////////////

Yast SearchThread::get_current_file()
{
    AcquireSRWLockExclusive(&m_lock);
    Yast cf(m_current_file);
    ReleaseSRWLockExclusive(&m_lock);
    return cf;
}

////////////////////////////////////////////////////////////////////////////////

void SearchThread::set_current_file(const Yast& current_file)
{
    AcquireSRWLockExclusive(&m_lock);
    m_current_file = current_file;
    ReleaseSRWLockExclusive(&m_lock);
}

////////////////////////////////////////////////////////////////////////////////

DWORD SearchThread::thread_proc(void* pctxt)
{
    SearchThread* self = p2p<SearchThread*>(pctxt);
    const SearchParams& params = self->m_params;

    YastSet backup_files;
    DirectoryIterator diter(params.search_path);
    const UINT prefix_len = diter.prefix_len();
    bool is_dir;
    Yast full_name;
    bool go_down = params.search_subdirs;
    while (diter.next(full_name, is_dir, go_down) && !self->m_canceled)
    {
        if (backup_files.find(full_name) != backup_files.end())
        {
            // do NOT search or count backup files!
            continue;
        }
        Yast name_only(diter.get_info()->cFileName);
        if (is_dir)
        {
            go_down = (
                params.search_subdirs &&
                !self->excl_dir(name_only, full_name)
                );
        }
        else
        {
            const bool include = self->incl_file(name_only);
            if (include)
            {
                self->search_file(full_name, prefix_len, backup_files);
            }
            params.next_cb(params.p_ctxt, include);
        }
    }

    params.end_search_cb(params.p_ctxt);
    InterlockedExchange(&self->m_canceled, false);
    InterlockedExchange(&self->m_running, false);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool SearchThread::excl_dir(const Yast& name_only, const Yast& full_name)
{
    range r;
    return (
        m_params.rx_exclude != nullptr && (
            m_params.rx_exclude->search(r, name_only) ||
            m_params.rx_exclude->search(r, full_name)
            )
        );
}

////////////////////////////////////////////////////////////////////////////////

bool SearchThread::incl_file(const Yast& name)
{
    if (m_params.rx_include)
    {
        range r;
        return m_params.rx_include->search(r, name);
    }
    else if (m_params.inc_patterns.size())
    {
        // Set 'include' according to the type of the first pattern.
        bool include = m_params.inc_patterns[0].str()[0] == '-';
        Yast cmp(name);
        cmp.to_lower();
        for (const Yast& pat : m_params.inc_patterns)
        {
            PCWSTR pstr = pat.str();
            if (pstr[0]  == '-')
            {
                include = include && !wild_match(cmp, pstr + 1);
            }
            else
            {
                include = include || wild_match(cmp, pstr);
            }
        }
        return include;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void SearchThread::search_file(
    const Yast& path,
    UINT prefix_len,
    YastSet& backup_files
    )
{
    const bool prefer_utf8 = true;
    set_current_file(path);
    TextFile tf;
    if (tf.load(path, prefer_utf8, m_params.search_binary))
    {
        const Yast& subject = tf.get_content();
        ranges match_ranges;
        range match;
        for (
            size_t pos = 0;
            !m_canceled && m_params.rx_search->search(match, subject, pos);
            pos = match.end
            )
        {
            match_ranges.push_back(match);
        }

        // Only try to replace, if our primary regex matched. Do not take
        // matches of rx_search_utf16 into account when deciding whether to
        // try to replace.
        const bool try_to_replace = (
            m_params.do_replace &&
            match_ranges.size() != 0
            );

        // search for literal utf16 in binary files
        if (m_params.rx_search_utf16 != nullptr)
        {
            for (
                size_t pos = 0;
                !m_canceled && m_params.rx_search_utf16->search(
                    match,
                    subject,
                    pos
                    );
                pos = match.end
                )
            {
                match_ranges.push_back(match);
            }
        }

        if (match_ranges.size())
        {
            // have to extract match info *before* replacing
            m_result.path = path;
            m_result.path_prefix_len = prefix_len;
            m_result.encoding = tf.get_encoding();
            m_result.line_info = tf.lines_from_ranges(match_ranges);

            if (!m_canceled && try_to_replace)
            {
                if (!do_replace(tf, backup_files))
                {
                    // replacing failed -> do not report match info
                    return;
                }
            }

            m_params.match_found_cb(
                m_params.p_ctxt,
                match_ranges.size(),
                m_result
                );
        }
    }
    set_current_file(Yast());
}

////////////////////////////////////////////////////////////////////////////////

bool SearchThread::do_replace(TextFile& txt_file, YastSet& backup_files)
{
    const Yast& subject = txt_file.get_content();
    Yast replaced(
        m_params.rx_search->replace(subject, m_params.replace_text)
        );
    if (!replaced.binary_same(subject))
    {
        const DWORD ATTR_TO_REMOVE = (
            FILE_ATTRIBUTE_HIDDEN |
            FILE_ATTRIBUTE_READONLY |
            FILE_ATTRIBUTE_SYSTEM
            );
        const Yast& path = txt_file.get_path();
        TRACE("Parts of the content have been replaced for\n%S\n", path.str());
        txt_file.move_to_content(replaced);
        if (m_params.create_backups)
        {
            Yast backup = path + L".bak";
            if (
                !CopyFile(path, backup, false) &&
                GetLastError() == ERROR_ACCESS_DENIED
                )
            {
                DWORD attr = GetFileAttributes(backup);
                attr &= ~ATTR_TO_REMOVE;
                SetFileAttributes(backup, attr);
                if (!CopyFile(path, backup, false))
                {
                    TRACE("Failed to create backup: %S\n", backup.str());
                    return false;
                }
            }
            backup_files.insert(backup);
        }
        if (!txt_file.store(path))
        {
            DWORD init_attr = GetFileAttributes(path);
            DWORD tmp_attr = init_attr & ~ATTR_TO_REMOVE;
            SetFileAttributes(path, tmp_attr);
            bool ok = txt_file.store(path);
            SetFileAttributes(path, init_attr);
            if (!ok)
            {
                TRACE("Failed to store: %S\n", path.str());
                return false;
            }
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
