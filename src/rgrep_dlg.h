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

#pragma once

#include "font_size_dlg.h"
#include "resize_dlg_layout.h"
#include "list_ctrl.h"
#include "rgrep_rx.h"
#include "auto_complete_cb.h"
#include "search_thread.h"

/////////////////////////////////////////////////////////////////////////////

class GrepDlg : public FontSizeDlg
{
public:
    GrepDlg(PCWSTR initial_dir = nullptr);
    INT_PTR DoModal(int font_size);
    ~GrepDlg()
    {
    }

private:
    SearchThread        m_thread;
    SearchResults       m_results;
    ResizeDlgLayout     m_layout;
    ListCtrl            m_result_list;
    AutoCompleteCombo   m_ac_path;
    AutoCompleteCombo   m_ac_regex;
    AutoCompleteCombo   m_ac_replace;
    AutoCompleteCombo   m_ac_exc_dirs;
    AutoCompleteCombo   m_ac_inc_files;
    Yast                m_editor_cmd;
    Yast                m_viewer_cmd;
    Yast                m_initial_path;
    Yast                m_csv_sep;
    HMENU               m_ctxt_menu;
    HWND                m_last_focus;
    UINT                m_search_flags;
    UINT                m_num_processed;
    UINT                m_num_searched;
    UINT                m_num_matches;
    UINT                m_num_file_matches;
    bool                m_create_backups;
    bool                m_search_regex;
    bool                m_include_regex;
    bool                m_search_subdirs;
    bool                m_search_binary;
    bool                m_search_rx_ok;
    bool                m_exclude_rx_ok;
    bool                m_include_rx_ok;
    bool                m_combo_popup_open;

    virtual INT_PTR OnMessage(UINT msg, WPARAM wp, LPARAM lp) override;
    virtual bool OnInitDialog() override;
    virtual bool OnCommand(UINT CmdId, UINT Notification, HWND Ctrl) override;
    virtual bool OnNotify(UINT CtrlId, NMHDR *pHdr) override;

    bool PrepareSearchParams(SearchParams& params, bool do_replace);
    void StartSearch(bool do_replace);
    void AddResult(SearchResult* result);

    void InitializePosition(WINDOWPLACEMENT* pwp);
    void InitializeResultList();
    bool LoadSettings(WINDOWPLACEMENT *pwp);
    void SaveSettings();
    bool PreTranslateMessage(MSG* pmsg);
    void CheckValidSearchText();
    void CheckValidExcludeDir();
    void CheckValidIncludeFile();
    INT_PTR OnCtrlColor(HWND ctrl, HDC hdc);
    void UpdateInfo(bool include_current = false);
    void AutoSizeColumns();
    bool OkToModifyWithoutBackups(const Yast& search, const Yast& replace);
    bool OnContextMenu(BaseWnd wnd, CPoint pt);
    void TrackComboPopupState(UINT Notification);
    void OpenFileFromList(int idx, bool in_explorer = false);
    void BrowseForSearchPath(Yast& path);

    enum ClipSource
    {
        cs_path,
        cs_filename,
        cs_text,
        cs_result,
        cs_csv_result,
    };
    void CopyToClipboard(ClipSource src);

    // Callbacks from search thread
    // These need to switch to the GUI thread.
    static void OnNext(void *pCtxt, bool was_searched);
    static void OnEndSearch(void *pCtxt);
    static void OnMatch(void *pCtxt, size_t num_matches, SearchResult& result);

    static const UINT WM_APP_FOUND_SEARCH = WM_APP + 0;
    static const UINT WM_APP_PROGRESS     = WM_APP + 1;
    static const UINT WM_APP_END_SEARCH   = WM_APP + 2;
    static const UINT WM_APP_HACK_INIT    = WM_APP + 3;

    static const UINT LABEL_TIMER = 1;
    static const UINT COL_NAME = 0;
    static const UINT COL_ENC  = 1;
    static const UINT COL_LINE = 2;
    static const UINT COL_TEXT = 3;

    static const UINT SYSM_PCRE = 1;
};
