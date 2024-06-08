////////////////////////////////////////////////////////////////////////////////
//
// This file is part of rgrep.
// rgrep is based on PCRE2 (see pcre2_16\LICENCE).
//
// Copyright 2018-2023 Rocco Matano
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

#include "rgrep_rx.h"
#include "text_file.h"

////////////////////////////////////////////////////////////////////////////////

struct SearchResult
{
    Yast            path;
    LineInfos       line_info;
    TextEncoding    encoding;
    UINT            path_prefix_len;
};

typedef cvector<SearchResult> SearchResults;

////////////////////////////////////////////////////////////////////////////////

typedef void (*NEXT_FILE_CB)(void *pCtxt, bool was_searched, PCWSTR name);
typedef void (*END_SEARCH_CB)(void *pCtxt);
typedef void (*MATCH_FOUND_CB)(
    void *pCtxt,
    size_t num_matches,
    SearchResult& result
    );

struct SearchParams
{
    Yast            search_path;
    Yast            replace_text;
    YastVector      inc_patterns;
    rrx::ptr        rx_search;
    rrx::ptr        rx_search_utf16;
    rrx::ptr        rx_exclude;
    rrx::ptr        rx_include;
    void*           p_ctxt;
    NEXT_FILE_CB    next_cb;
    MATCH_FOUND_CB  match_found_cb;
    END_SEARCH_CB   end_search_cb;
    bool            search_subdirs;
    bool            search_binary;
    bool            do_replace;
    bool            create_backups;
};

////////////////////////////////////////////////////////////////////////////////

class SearchThread
{
public:
    SearchThread();
    ~SearchThread();
    bool start(SearchParams& params);
    void cancel();
    bool is_running();

protected:
    SearchParams        m_params;
    SearchResult        m_result;
    volatile LONG       m_running;
    volatile LONG       m_canceled;

    typedef cset<Yast> YastSet;

    static DWORD WINAPI thread_proc(void* pctxt);
    bool excl_dir(const Yast& name);
    bool incl_file(const Yast& name);
    void search_file(const Yast& path, UINT prefix_len, YastSet& backup_files);
    bool do_replace(TextFile& txt_file, YastSet& backup_files);
};

////////////////////////////////////////////////////////////////////////////////
