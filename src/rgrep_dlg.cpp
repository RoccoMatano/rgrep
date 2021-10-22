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
#include "rgrep_dlg.h"
#include "sys_icon.h"
#include "res/resource.h"
#include "settings_dlg.h"

////////////////////////////////////////////////////////////////////////////////

GrepDlg::GrepDlg(PCWSTR initial_dir) :
    FontSizeDlg(nullptr),
    m_initial_path(initial_dir),
    m_csv_sep(L","),
    m_ctxt_menu(nullptr),
    m_search_flags(0),
    m_create_backups(false),
    m_search_regex(false),
    m_include_regex(false),
    m_search_subdirs(false),
    m_search_binary(false),
    m_search_rx_ok(true),
    m_exclude_rx_ok(true),
    m_include_rx_ok(true)
{
}

////////////////////////////////////////////////////////////////////////////////

static PCWSTR const rgrep_reg = L"RoMa\\rgrep";
static PCWSTR const rgrep_reg_hist = L"RoMa\\rgrep\\history";

bool GrepDlg::LoadSettings(WINDOWPLACEMENT *pwp)
{
    m_ac_path.load(rgrep_reg_hist, L"search_path_");
    m_ac_regex.load(rgrep_reg_hist, L"search_for_");
    m_ac_replace.load(rgrep_reg_hist, L"replace_with_");
    m_ac_exc_dirs.load(rgrep_reg_hist, L"exclude_dir_");
    m_ac_inc_files.load(rgrep_reg_hist, L"include_file_");

    RegAutoKey rkey(rgrep_reg);
    if (!rkey) return false;

    ReadRegDword(rkey, L"search_flags", m_search_flags);
    ReadRegBool(rkey, L"regex_search", m_search_regex);
    ReadRegBool(rkey, L"create_backups", m_create_backups);
    ReadRegBool(rkey, L"regex_include", m_include_regex);
    ReadRegBool(rkey, L"search_subdirs", m_search_subdirs);
    ReadRegBool(rkey, L"search_binary", m_search_binary);
    ReadRegString(rkey, L"editor_cmd", m_editor_cmd);
    ReadRegString(rkey, L"viewer_cmd", m_viewer_cmd);
    ReadRegString(rkey, L"csv_sep", m_csv_sep);

    if (ReadRegBinary(rkey, L"placement", pwp, sizeof(*pwp)))
    {
        if (pwp->length == sizeof(*pwp))
        {
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::SaveSettings()
{
    m_ac_path.save();
    m_ac_regex.save();
    m_ac_replace.save();
    m_ac_exc_dirs.save();
    m_ac_inc_files.save();

    RegAutoKey rkey(rgrep_reg);
    if (!rkey) return;

    WriteRegDword(rkey, L"search_flags", m_search_flags);
    WriteRegDword(rkey, L"regex_search", m_search_regex);
    WriteRegDword(rkey, L"create_backups", m_create_backups);
    WriteRegDword(rkey, L"regex_include", m_include_regex);
    WriteRegDword(rkey, L"search_subdirs", m_search_subdirs);
    WriteRegDword(rkey, L"search_binary", m_search_binary);
    WriteRegString(rkey, L"editor_cmd", m_editor_cmd);
    WriteRegString(rkey, L"viewer_cmd", m_viewer_cmd);
    WriteRegString(rkey, L"csv_sep", m_csv_sep);

    WINDOWPLACEMENT wp { sizeof(wp) };
    GetWindowPlacement(m_hWnd, &wp);
    WriteRegBinary(rkey, L"placement", &wp, sizeof(wp));
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR GrepDlg::DoModal(int font_size)
{
    // Use a dedicated message pump to be able to peek into messages for
    // controls (with PreTranslateMessage).

    CreateModeless(IDD_RGREP, font_size);
    Show();
    BringToTop();
    SetForeground();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!PreTranslateMessage(&msg))
        {
            if (!IsDialogMessage(m_hWnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    Destroy();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool GrepDlg::OnInitDialog()
{
    HICON hi = LoadIconW(Hinstance(), MAKEINTRESOURCEW(IDI_RGREP));
    SendMessage(WM_SETICON, 0, p2lp(hi));
    SendMessage(WM_SETICON, 1, p2lp(hi));

    static const WCHAR pcre_lic[] = L"rgrep is based on PCRE2";
    MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.dwTypeData = const_cast<PWSTR>(pcre_lic);
    mii.cch = sizeof(pcre_lic) - 1;
    mii.wID = SYSM_PCRE;
    HMENU smenu = GetSystemMenu(m_hWnd, false);
    InsertMenuItem(smenu, ~0u, true, &mii);

    // have to connect auto complete controls before calling LoadSettings
    bool ok = true;
    ok &= m_ac_path.connect(GetItem(IDC_SEARCH_PATH));
    ok &= m_ac_regex.connect(GetItem(IDC_SEARCH_TEXT));
    ok &= m_ac_replace.connect(GetItem(IDC_REPLACE_TEXT));
    ok &= m_ac_exc_dirs.connect(GetItem(IDC_EXCLUDE_PATTERN));
    ok &= m_ac_inc_files.connect(GetItem(IDC_INCLUDE_PATTERN));
    TRACE("ac connect %s\n", ok ? "ok" : "FAILED!");

    WINDOWPLACEMENT wp;
    bool wpok = LoadSettings(&wp);

    // set focus to search text
    SetFocus(m_ac_regex);

    // The standard item height in the popup windows of a combo box is quite
    // tiny (most likely because the combo box code is from a distant time in
    // the past, when screen area was more precious).
    // So we increase the item height to make the text more readable.
    m_ac_path.scale_item_height(4, 3);
    m_ac_regex.scale_item_height(4, 3);
    m_ac_replace.scale_item_height(4, 3);
    m_ac_exc_dirs.scale_item_height(4, 3);
    m_ac_inc_files.scale_item_height(4, 3);

    m_ctxt_menu = LoadMenu(Hinstance(), MAKEINTRESOURCE(IDM_RESULT_LIST));
    m_ctxt_menu = GetSubMenu(m_ctxt_menu, 0);
    CheckButton(IDC_RADIO_REGEX, m_search_regex);
    CheckButton(IDC_RADIO_LITERAL, !m_search_regex);
    CheckButton(IDC_INCLUDE_REGEX, m_include_regex);
    CheckButton(IDC_INCLUDE_WILDCARD, !m_include_regex);
    CheckButton(IDC_CASE_SENSITIVE, 0 == (m_search_flags & rrx::IGNORE_CASE));
    CheckButton(IDC_WHOLE_WORD, 0 != (m_search_flags & rrx::WHOLE_WORDS));
    CheckButton(IDC_DOT_ALL, 0 != (m_search_flags & rrx::DOT_ALL));
    CheckButton(IDC_MULTI_LINE, 0 != (m_search_flags & rrx::MULTI_LINE));
    CheckButton(IDC_CREATE_BACKUP, m_create_backups);
    CheckButton(IDC_SEARCH_SUBFOLDERS, m_search_subdirs);
    CheckButton(IDC_SEARCH_BINARY, m_search_binary);
    CheckValidSearchText();
    CheckValidExcludeDir();
    CheckValidIncludeFile();

    // enanble AutoComplete after content of the controls has been set.
    ok = true;
    ok &= m_ac_path.enable(true);
    ok &= m_ac_regex.enable(true);
    ok &= m_ac_replace.enable(true);
    ok &= m_ac_exc_dirs.enable(true);
    ok &= m_ac_inc_files.enable(true);
    TRACE("ac enable %s\n", ok ? "ok" : "FAILED!");

    InitializeResultList();
    InitializePosition(wpok ? &wp : nullptr);

    // Actually, now would be the time to set the edit control of m_ac_path
    // to the initial path. But doing so now makes the combo box performing
    // it matching code, where it replaces the text in the edit control with
    // one of its list items, should those have the same prefix.
    // The only way I found to circumvent this, is to delay this initialization
    // to some time in the future. We do that by posting a message.
    PostMessage(WM_APP_HACK_INIT, 0, 0);

    // We already set focus to m_ac_regex and do not want the dialog
    // manager to interfere. So we have to return false.
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::InitializeResultList()
{
    m_result_list = GetItem(IDC_RESULT_LIST);
    m_result_list.SetImageList(SysIconIdx::img_list(), LVSIL_SMALL);

    LVCOLUMN column;
    column.mask = LVCF_FMT | LVCF_TEXT;
    column.fmt = LVCFMT_LEFT;

    column.pszText = const_cast<PWSTR>(L"Name");
    m_result_list.InsertColumn(COL_NAME, column);

    column.pszText = const_cast<PWSTR>(L"Enc");
    m_result_list.InsertColumn(COL_ENC, column);

    column.pszText = const_cast<PWSTR>(L"Line");
    m_result_list.InsertColumn(COL_LINE, column);

    column.pszText = const_cast<PWSTR>(L"Text");
    m_result_list.InsertColumn(COL_TEXT, column);

    m_result_list.SetStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    AutoSizeColumns();
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR GrepDlg::OnCtrlColor(HWND ctrl, HDC hdc)
{
    bool valid = true;
    if (ctrl == m_ac_regex.get_edit())
    {
        valid = m_search_rx_ok;
    }
    else if (ctrl == m_ac_exc_dirs.get_edit())
    {
        valid = m_exclude_rx_ok;
    }
    else if (ctrl == m_ac_inc_files.get_edit())
    {
        valid = m_include_rx_ok;
    }

    // WM_CTLCOLOR* messages belong to the small number of messages
    // for which a dialog does not set DWLP_MSGRESULT but returns
    // the message result directly (see documentation for
    // DialogProc).
    if (valid)
    {
        return false;
    }
    COLORREF bk = RGB(255, 200, 200);
    SetBkColor(hdc, bk);
    SetDCBrushColor(hdc, bk);
    return p2i<INT_PTR>(GetStockObject(DC_BRUSH));
}

////////////////////////////////////////////////////////////////////////////////

bool GrepDlg::PreTranslateMessage(MSG* pmsg)
{
    // Add some custom keyboard shortcuts:
    //
    //      Result list: 'Enter' opens first selected file, 'Ctrl+A' selects
    //                   all entries.
    //      Popup of auto complete combo boxes: 'Del' removes selected entry.

    if (pmsg->message == WM_KEYDOWN)
    {
        UINT const key = static_cast<UINT>(pmsg->wParam);
        HWND const hwnd = pmsg->hwnd;
        if (hwnd == m_result_list)
        {
            if (key == VK_RETURN)
            {
                const UINT num_sel = m_result_list.GetSelectedCount();
                const int idx = m_result_list.GetSelectionMark();
                if (num_sel < 2 && idx >= 0)
                {
                    OpenFileFromList(idx);
                    return true;
                }
            }
            else if (key == 'A' && GetKeyState(VK_CONTROL) < 0)
            {
                const int cnt = m_result_list.GetItemCount();
                for (int i = 0; i < cnt; i++)
                {
                    m_result_list.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
                }
                return true;
            }
        }
        else if (key == VK_DELETE && m_combo_popup_open)
        {
            AutoCompleteCombo *pac = nullptr;
            if (hwnd == m_ac_path.get_edit())
            {
                pac = &m_ac_path;
            }
            else if (hwnd == m_ac_regex.get_edit())
            {
                pac = &m_ac_regex;
            }
            else if (hwnd == m_ac_replace.get_edit())
            {
                pac = &m_ac_replace;
            }
            else if (hwnd == m_ac_exc_dirs.get_edit())
            {
                pac = &m_ac_exc_dirs;
            }
            else if (hwnd == m_ac_inc_files.get_edit())
            {
                pac = &m_ac_inc_files;
            }
            if (pac)
            {
                pac->remove_selected();
                return true;;
            }
        }
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////

INT_PTR GrepDlg::OnMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_SIZE:
            m_layout.OnSize(LOWORD(lp), HIWORD(lp));
            return false;

        case WM_GETMINMAXINFO:
            m_layout.OnMinMaxInfo(i2p<MINMAXINFO*>(lp));
            return true;

        case WM_CONTEXTMENU:
            return OnContextMenu(i2p<HWND>(wp), CPoint(lp));

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
            return OnCtrlColor(i2p<HWND>(lp), i2p<HDC>(wp));

        case WM_CLOSE:
            if (m_thread.is_running())
            {
                m_thread.cancel();
                // Swallow message by returning true, so that it does not
                // get converted to a IDCANCEL.
                return true;
            }
            return false;

        case WM_TIMER:
            UpdateInfo(true);
            return true;

        case WM_SYSCOMMAND:
            if (wp == SYSM_PCRE)
            {
                static const WCHAR url[] = L"https://www.pcre.org/licence.txt";
                ShellExecute(m_hWnd, nullptr, url, nullptr, nullptr, SW_SHOW);
                return true;
            }
            return false;

        case WM_APP_FOUND_SEARCH:
            m_num_file_matches += 1;
            m_num_matches += static_cast<UINT>(wp);
            AddResult(i2p<SearchResult*>(lp));
            UpdateInfo();
            return true;

        case WM_APP_PROGRESS:
            if (wp)
            {
                m_num_searched++;
            }
            m_num_processed++;
            return true;

        case WM_APP_END_SEARCH:
        {
            BaseWnd progress(GetItem(IDC_PROGRESS));
            progress.SendMessage(PBM_SETMARQUEE, 0, 0);
            progress.ModifyStyle(PBS_MARQUEE, 0);
            progress.SendMessage(PBM_SETPOS, 0, 0);
            UpdateInfo();
            GetItem(IDC_DO_SEARCH).SetText(L"&Search");
            KillTimer(LABEL_TIMER);
            AutoSizeColumns();
            SetFocus(m_last_focus);
            return true;
        }

        case WM_APP_HACK_INIT:
            // init content of the 'path' edit control
            TRACE("init path: %S\n", m_initial_path.str());
            if (m_initial_path.is_empty())
            {
                // replace with MRU path
                m_initial_path = m_ac_path.item_str(0);
            }
            m_ac_path.SendMessage(WM_SETTEXT, 0, m_initial_path);
            // update caption
            OnCommand(IDC_SEARCH_PATH, CBN_EDITCHANGE, GetItem(IDC_SEARCH_PATH));

            #if 0
            // init content of the 'search text' edit control MRU search text
            m_ac_regex.SendMessage(WM_SETTEXT, 0, m_ac_regex.item_str(0));
            // and select the text
            m_ac_regex.SendMessage(CB_SETEDITSEL, 0, MAKELONG(0, -1));
            #endif

            return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::OpenFileFromList(int idx, bool in_explorer)
{
    WCHAR line_no[64];
    LVITEM lvi;
    lvi.iItem = idx;
    lvi.iSubItem = COL_LINE;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = line_no;
    lvi.cchTextMax = ARRAYSIZE(line_no);
    if (m_result_list.GetItem(lvi))
    {
        UINT ridx = static_cast<UINT>(lvi.lParam);
        PCWSTR path = m_results[ridx].path;
        const bool is_binary = (m_results[ridx].encoding == TE_BINARY);
        PCWSTR cmd = nullptr;
        if (!is_binary && m_editor_cmd.find(L"%path%") >= 0)
        {
            cmd = m_editor_cmd;
        }
        else if (is_binary && m_viewer_cmd.find(L"%path%") >= 0)
        {
            cmd = m_viewer_cmd;
        }

        if (in_explorer || cmd == nullptr)
        {
            Yast param_str;
            PCWSTR param = nullptr;
            if (in_explorer)
            {
                param_str.format(L"/Select,\"%s\"", path);
                path = L"explorer.exe";
                param = param_str;
            }
            ShellExecute(
                m_hWnd,
                nullptr,
                path,
                param,
                nullptr,
                SW_SHOW
                );
        }
        else
        {
            Yast ext_cmd(cmd);
            ext_cmd.replace(L"%path%", path);
            ext_cmd.replace(L"%line%", line_no);
            TRACE("ext_cmd: '%S'\n", ext_cmd.str());

            STARTUPINFO si = {sizeof(STARTUPINFO)} ;
            PROCESS_INFORMATION pi;
            CreateProcess(
                nullptr,
                static_cast<PWSTR>(ext_cmd),
                nullptr,
                nullptr,
                false,
                0,
                0,
                nullptr,
                &si,
                &pi
                );
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::CopyToClipboard(ClipSource src)
{
    Yast clip, last;
    int str_idx;
    int idx = m_result_list.GetNextItem(-1, LVNI_SELECTED);
    while (idx >= 0)
    {
        size_t ridx = m_result_list.GetItemData(idx);
        Yast add(m_results[ridx].path);
        // src == cs_path is already handled by line above
        if (src == cs_result)
        {
            add.format(
                L"%s, %s, %s, %s",
                m_result_list.GetItemText(idx, COL_NAME).str(),
                m_result_list.GetItemText(idx, COL_ENC).str(),
                m_result_list.GetItemText(idx, COL_LINE).str(),
                m_result_list.GetItemText(idx, COL_TEXT).str()
                );
        }
        else if (src == cs_csv_result)
        {
            PCWSTR sep = m_csv_sep.str();
            add.format(
                L"%s%s%s%s%s%s%s",
                m_result_list.GetItemText(idx, COL_NAME).str(),
                sep,
                m_result_list.GetItemText(idx, COL_ENC).str(),
                sep,
                m_result_list.GetItemText(idx, COL_LINE).str(),
                sep,
                m_result_list.GetItemText(idx, COL_TEXT).str()
                );
        }
        else if (src == cs_filename)
        {
            str_idx = add.rfind(L"\\");
            str_idx = (str_idx < 0) ? 0 : str_idx + 1;
            add = add.slice(str_idx, -1);
        }
        else if (src == cs_text)
        {
            add = m_result_list.GetItemText(idx, COL_TEXT);
        }
        if (src == cs_text || add != last)
        {
            if (!clip.is_empty())
            {
                clip += L"\r\n";
            }
            clip += add;
            last = std::move(add);
        }
        idx = m_result_list.GetNextItem(idx, LVNI_SELECTED);
    }
    clip.to_clipboard(m_hWnd);
}

////////////////////////////////////////////////////////////////////////////////

bool GrepDlg::OnNotify(UINT CtrlId, NMHDR *phdr)
{
    if (CtrlId == IDC_RESULT_LIST)
    {
        NMITEMACTIVATE *pia = p2p<NMITEMACTIVATE*>(phdr);
        if (pia->hdr.code == NM_DBLCLK)
        {
            OpenFileFromList(pia->iItem);
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool GrepDlg::OnContextMenu(BaseWnd wnd, CPoint pt)
{
    const UINT num_sel = m_result_list.GetSelectedCount();
    if (wnd != m_result_list || num_sel < 1)
    {
        return false;
    }
    const UINT en_single = MF_BYCOMMAND | (
        (num_sel < 2) ? MF_ENABLED : MF_DISABLED
        );
    EnableMenuItem(m_ctxt_menu, IDC_OPEN_EDITOR, en_single);
    EnableMenuItem(m_ctxt_menu, IDC_OPEN_FOLDER, en_single);

    if (pt.x == -1 || pt.y == -1)
    {
        const int idx = m_result_list.GetSelectionMark();
        CRect rc = m_result_list.GetSubItemRect(idx, COL_NAME);
        pt.x = (rc.left + rc.right) / 2;
        pt.y = (rc.top + rc.bottom) / 2;
        m_result_list.ClientToScreen(pt);
    }
    SetForeground();
    TrackPopupMenu(
        m_ctxt_menu,
        TPM_LEFTALIGN | TPM_TOPALIGN,
        pt.x,
        pt.y,
        0,
        m_hWnd,
        0
        );
    PostMessage(WM_NULL, 0, 0);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::TrackComboPopupState(UINT Notification)
{
    if (Notification == CBN_DROPDOWN)
    {
        m_combo_popup_open = true;
    }
    else if (Notification != CBN_SELCHANGE)
    {
        m_combo_popup_open = false;
    }
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::CheckValidSearchText()
{
    m_search_rx_ok = (
        !m_search_regex ||
        (rrx::compile(m_ac_regex.GetText()) != nullptr)
        );
    GetItem(IDC_DO_SEARCH).Enable(m_search_rx_ok);
    GetItem(IDC_DO_REPLACE).Enable(m_search_rx_ok);
    m_ac_regex.InvalidateRect();
    m_ac_regex.Update();
}

void GrepDlg::CheckValidExcludeDir()
{
    m_exclude_rx_ok = rrx::compile(m_ac_exc_dirs.GetText()) != nullptr;
    m_ac_exc_dirs.InvalidateRect();
    m_ac_exc_dirs.Update();
}

void GrepDlg::CheckValidIncludeFile()
{
    m_include_rx_ok = (
        !m_include_regex ||
        (rrx::compile(m_ac_inc_files.GetText()) != nullptr)
        );
    m_ac_inc_files.InvalidateRect();
    m_ac_inc_files.Update();
}

////////////////////////////////////////////////////////////////////////////////

bool GrepDlg::OnCommand(UINT CmdId, UINT Notification, HWND Ctrl)
{
    switch (CmdId)
    {
        case IDC_DO_SEARCH:
        case IDC_DO_REPLACE:
            if (m_thread.is_running())
            {
                m_thread.cancel();
            }
            else
            {
                StartSearch(CmdId == IDC_DO_REPLACE);
            }
            break;

        case IDCANCEL:
            if (m_thread.is_running())
            {
                m_thread.cancel();
            }
            else
            {
                SaveSettings();
                PostQuitMessage(0);
            }
            break;

        case IDC_RADIO_REGEX:
        case IDC_RADIO_LITERAL:
            m_search_regex = IsButtonChecked(IDC_RADIO_REGEX);
            CheckValidSearchText();
            break;

        case IDC_INCLUDE_REGEX:
        case IDC_INCLUDE_WILDCARD:
            m_include_regex = IsButtonChecked(IDC_INCLUDE_REGEX);
            CheckValidIncludeFile();
            break;

        case IDC_CASE_SENSITIVE:
            if (IsButtonChecked(IDC_CASE_SENSITIVE))
            {
                m_search_flags &= ~rrx::IGNORE_CASE;
            }
            else
            {
                m_search_flags |= rrx::IGNORE_CASE;
            }
            break;

        case IDC_WHOLE_WORD:
            if (IsButtonChecked(IDC_WHOLE_WORD))
            {
                m_search_flags |= rrx::WHOLE_WORDS;
            }
            else
            {
                m_search_flags &= ~rrx::WHOLE_WORDS;
            }
            break;

        case IDC_DOT_ALL:
            if (IsButtonChecked(IDC_DOT_ALL))
            {
                m_search_flags |= rrx::DOT_ALL;
            }
            else
            {
                m_search_flags &= ~rrx::DOT_ALL;
            }
            break;

        case IDC_MULTI_LINE:
            if (IsButtonChecked(IDC_MULTI_LINE))
            {
                m_search_flags |= rrx::MULTI_LINE;
            }
            else
            {
                m_search_flags &= ~rrx::MULTI_LINE;
            }
            break;

        case IDC_SEARCH_SUBFOLDERS:
            m_search_subdirs = IsButtonChecked(IDC_SEARCH_SUBFOLDERS);
            break;

        case IDC_SEARCH_BINARY:
            m_search_binary = IsButtonChecked(IDC_SEARCH_BINARY);
            break;

        case IDC_CREATE_BACKUP:
            m_create_backups = IsButtonChecked(IDC_CREATE_BACKUP);
            break;

        case IDC_SEARCH_TEXT:
            //TRACE("CBN: %u\n", Notification);
            TrackComboPopupState(Notification);
            CheckValidSearchText();
            break;

        case IDC_EXCLUDE_PATTERN:
            TrackComboPopupState(Notification);
            CheckValidExcludeDir();
            break;

        case IDC_INCLUDE_PATTERN:
            TrackComboPopupState(Notification);
            CheckValidIncludeFile();
            break;

        case IDC_SEARCH_PATH:
            TrackComboPopupState(Notification);
            if (Notification == CBN_EDITCHANGE || Notification == CBN_SELCHANGE)
            {
                Yast pth(m_ac_path.GetText());
                WCHAR compact[80];
                PathCompactPathEx(compact, pth, 70, 0);
                pth.format(L"rgrep: %s", compact);
                SetText(pth);
            }
            break;

        case IDC_SETTINGS:
        {
            SettingsDlg sd(this, m_editor_cmd, m_viewer_cmd, m_csv_sep);
            if (sd.DoModal() == IDOK)
            {
                m_editor_cmd = sd.get_edit_cmd();
                m_viewer_cmd = sd.get_view_cmd();
                m_csv_sep = sd.get_csv_sep();
            }
            break;
        }

        case IDC_SEARCH_PATH_BROWSE:
        {
            Yast dir(m_ac_path.GetText());
            BrowseForSearchPath(dir);
            m_ac_path.SetText(dir);
            break;
        }

        case IDC_OPEN_EDITOR:
            OpenFileFromList(m_result_list.GetSelectionMark());
            break;

        case IDC_OPEN_FOLDER:
            OpenFileFromList(m_result_list.GetSelectionMark(), true);
            break;

        case IDC_COPY_PATH:
            CopyToClipboard(cs_path);
            break;

        case IDC_COPY_FILENAME:
            CopyToClipboard(cs_filename);
            break;

        case IDC_COPY_TEXT:
            CopyToClipboard(cs_text);
            break;

        case IDC_COPY_RESULT:
            CopyToClipboard(cs_result);
            break;

        case IDC_COPY_CSV_RESULT:
            CopyToClipboard(cs_csv_result);
            break;

    }
    return true;
    UNUSED(Ctrl);
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::BrowseForSearchPath(Yast& path)
{
    IFileOpenDialog *pfd;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pfd)
        );
    if (SUCCEEDED(hr))
    {
        pfd->SetOptions(
            FOS_NOCHANGEDIR |
            FOS_FORCEFILESYSTEM |
            FOS_PATHMUSTEXIST |
            FOS_DONTADDTORECENT |
            FOS_PICKFOLDERS
            );

        IShellItem* folder = nullptr;
        SHCreateItemFromParsingName(path, nullptr, IID_PPV_ARGS(&folder));
        if (folder)
        {
            pfd->SetFolder(folder);
            folder->Release();
        }

        if (SUCCEEDED(pfd->Show(m_hWnd)))
        {
            IShellItem *item = nullptr;
            pfd->GetResult(&item);
            if (item)
            {
                PWSTR pFilePath = nullptr;
                item->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath);
                path = pFilePath;
                CoTaskMemFree(pFilePath);
                item->Release();
            }
        }
        pfd->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::InitializePosition(WINDOWPLACEMENT* pwp)
{
    static const ResizeDlgLayout::CtrlAnchor anchors[] = {
        { IDC_CASE_SENSITIVE,       AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_CREATE_BACKUP,        AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_DOT_ALL,              AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_DO_REPLACE,           AP_TOPRIGHT,   AP_TOPRIGHT,    false },
        { IDC_DO_SEARCH,            AP_TOPRIGHT,   AP_TOPRIGHT,    false },
        { IDC_EXCLUDE_LABEL,        AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_EXCLUDE_PATTERN,      AP_TOPLEFT,    AP_TOPMIDDLE,   false },
        { IDC_INCLUDE_LABEL,        AP_TOPMIDDLE,  AP_TOPMIDDLE,   false },
        { IDC_INCLUDE_PATTERN,      AP_TOPMIDDLE,  AP_TOPRIGHT,    false },
        { IDC_INCLUDE_REGEX,        AP_TOPMIDDLE,  AP_TOPMIDDLE,   false },
        { IDC_INCLUDE_WILDCARD,     AP_TOPMIDDLE,  AP_TOPMIDDLE,   false },
        { IDC_SCOPE_GROUP,          AP_TOPLEFT,    AP_TOPRIGHT,    false },
        { IDC_MULTI_LINE,           AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_PROGRESS,             AP_TOPLEFT,    AP_TOPRIGHT,    false },
        { IDC_RADIO_LITERAL,        AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_RADIO_REGEX,          AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_REPLACE_TEXT,         AP_TOPLEFT,    AP_TOPRIGHT,    false },
        { IDC_REPLACE_TEXT_LABEL,   AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_RESULT_LIST,          AP_TOPLEFT,    AP_BOTTOMRIGHT, false },
        { IDC_SEARCH_BINARY,        AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_SEARCH_GROUP,         AP_TOPLEFT,    AP_TOPRIGHT,    false },
        { IDC_SEARCH_INFO,          AP_BOTTOMLEFT, AP_BOTTOMRIGHT, false },
        { IDC_LOCATION_GROUP,       AP_TOPLEFT,    AP_TOPRIGHT,    false },
        { IDC_SEARCH_PATH,          AP_TOPLEFT,    AP_TOPRIGHT,    false },
        { IDC_SEARCH_PATH_BROWSE,   AP_TOPRIGHT,   AP_TOPRIGHT,    false },
        { IDC_SEARCH_RESULTS_GROUP, AP_TOPLEFT,    AP_BOTTOMRIGHT, false },
        { IDC_SEARCH_SUBFOLDERS,    AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_SEARCH_TEXT,          AP_TOPLEFT,    AP_TOPRIGHT,    false },
        { IDC_SERACH_FOR_LABEL,     AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_SETTINGS,             AP_TOPLEFT,    AP_TOPLEFT,     false },
        { IDC_WHOLE_WORD,           AP_TOPLEFT,    AP_TOPLEFT,     false },
        };
    m_layout.Initialize(m_hWnd);
    m_layout.AnchorControls(anchors, ARRAYSIZE(anchors));

    if (pwp)
    {
        SetWindowPlacement(m_hWnd, pwp);
    }
    else
    {
        CRect WorkArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &WorkArea, 0);
        CRect Dlg(WindowRect());
        CPoint wac(WorkArea.CenterPoint());
        SetPos(
            nullptr,
            wac.x - Dlg.Width() / 2,
            wac.y - Dlg.Height() / 2,
            -1,
            -1,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
            );
    }
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::UpdateInfo(bool include_current)
{
    static const WCHAR fmt_nc[] = L"Searched %u files, skipped %u files."
        L" Found %u matches in %u files.";
    static const WCHAR fmt_ic[] = L" Scanning file '%s'.";
    Yast out;
    out.format(
        fmt_nc,
        m_num_processed,
        m_num_processed - m_num_searched,
        m_num_matches,
        m_num_file_matches
        );
    if (include_current)
    {
        Yast current_file = m_thread.get_current_file();
        if (!current_file.is_empty())
        {
            Yast scan;
            scan.format(fmt_ic, current_file.str());
            out += scan;
        }
    }
    GetItem(IDC_SEARCH_INFO).SetText(out);
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::AutoSizeColumns()
{
    const int cnt = m_result_list.GetColumnCount();
    for (int i = 0; i < cnt; i++)
    {
        m_result_list.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

////////////////////////////////////////////////////////////////////////////////

bool GrepDlg::OkToModifyWithoutBackups(const Yast& search, const Yast& replace)
{
    Yast ui_rep;
    if (replace.is_empty())
    {
        ui_rep = L"an empty string";
    }
    else
    {
        ui_rep.format(L"'%s'", replace.str());
    }

    Yast content;
    content.format(
        L"'%s'\n\nwill be replaced by\n\n%s.\n",
        search.str(),
        ui_rep.str()
        );

    TASKDIALOGCONFIG tdc = { sizeof(tdc) };
    tdc.hwndParent = m_hWnd;
    tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    tdc.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
    tdc.pszWindowTitle = L"rgrep";
    tdc.pszMainIcon = TD_WARNING_ICON;
    tdc.pszMainInstruction = L"Do you want to modify file content without "
        L"creating backups?";
    tdc.pszContent = content.str();
    tdc.nDefaultButton = IDNO;
    int pressed = 0;
    TaskDialogIndirect(&tdc, &pressed, nullptr, nullptr);
    return (pressed == IDYES);
}

////////////////////////////////////////////////////////////////////////////////

bool GrepDlg::PrepareSearchParams(SearchParams& params, bool do_replace)
{
    CheckValidSearchText();
    CheckValidExcludeDir();
    CheckValidIncludeFile();

    params.search_path = m_ac_path.GetText();
    params.replace_text = m_ac_replace.GetText();
    Yast  search_text(m_ac_regex.GetText());
    Yast exclude_text(m_ac_exc_dirs.GetText());
    Yast include_text(m_ac_inc_files.GetText());

    TRACE("path: %S\n", params.search_path.str());
    TRACE("search: %S\n", search_text.str());
    TRACE("replace: %S\n", params.replace_text.str());

    if (params.search_path.is_empty() || search_text.is_empty())
    {
        TRACE("path, text or both are empty\n");
        return false;
    }
    const UINT flags = m_search_flags | (m_search_regex ? 0: rrx::LITERAL);
    params.rx_search = rrx::compile(search_text, flags);
    if (!params.rx_search)
    {
        TRACE("prep '%S' does not compile\n", search_text.str());
        return false;
    }

    if (do_replace && !m_create_backups)
    {
        if (!OkToModifyWithoutBackups(search_text, params.replace_text))
        {
            return false;
        }
    }

    params.rx_exclude = nullptr;
    if (!exclude_text.is_empty())
    {
        params.rx_exclude = rrx::compile(exclude_text, rrx::IGNORE_CASE);
    }
    params.rx_include = nullptr;
    params.inc_patterns.clear();
    if (!include_text.is_empty())
    {
        if (m_include_regex)
        {
            params.rx_include = rrx::compile(include_text, rrx::IGNORE_CASE);
        }
        else
        {
            params.inc_patterns = include_text.split(L"|");
            for (auto& s : params.inc_patterns)
            {
                s.to_lower();
            }
        }
    }

    if (!m_search_regex && m_search_binary)
    {
        Yast search_text_utf16(2 * search_text.length());
        PWSTR pstu = search_text_utf16;
        PCWSTR pst = search_text;
        for (UINT i = 0; i < search_text.length(); i++)
        {
            pstu[2 * i] = 0;
            pstu[2 * i + 1] = pst[i];
        }
        // only rrx::LITERAL and possibly rrx::IGNORE_CASE matter in this case
        const UINT uflags = rrx::LITERAL | (m_search_flags & rrx::IGNORE_CASE);
        params.rx_search_utf16 = rrx::compile(search_text_utf16, uflags);
    }
    else
    {
        params.rx_search_utf16 = nullptr;
    }

    m_ac_path.add_entry(params.search_path);
    m_ac_regex.add_entry(search_text);
    m_ac_replace.add_entry(params.replace_text);
    m_ac_exc_dirs.add_entry(exclude_text);
    m_ac_inc_files.add_entry(include_text);

    params.p_ctxt = this;
    params.next_cb = OnNext;
    params.match_found_cb = OnMatch;
    params.end_search_cb = OnEndSearch;
    params.search_subdirs = m_search_subdirs;
    params.search_binary = m_search_binary;
    params.do_replace = do_replace;
    params.create_backups = m_create_backups;

    TRACE("params ok!\n");
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::StartSearch(bool do_replace)
{
    SearchParams params;
    if (!PrepareSearchParams(params, do_replace))
    {
        return;
    }

    m_last_focus = GetFocus();
    SetFocus(GetDlgItem(IDC_DO_SEARCH));
    SaveSettings();

    m_results.clear();
    GetItem(IDC_DO_SEARCH).SetText(L"&Stop");
    BaseWnd progress(GetItem(IDC_PROGRESS));
    progress.ModifyStyle(0, PBS_MARQUEE);
    progress.SendMessage(PBM_SETMARQUEE, 1, 0);
    m_result_list.DeleteAllItems();

    m_num_processed = m_num_searched = 0;
    m_num_matches = m_num_file_matches = 0;
    UpdateInfo();
    SetTimer(LABEL_TIMER, 200);

    if (!m_thread.start(params))
    {
        TRACE("failed to create thread\n");
        SendMessage(WM_APP_END_SEARCH, 0, 0);
    }
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::AddResult(SearchResult* result)
{
    m_results.push_back(*result);
    m_result_list.SendMessage(WM_SETREDRAW, false, 0);

    PCWSTR disp_name = result->path.str() + result->path_prefix_len;
    PCWSTR enc = L"?";
    switch (result->encoding)
    {
        case TE_BINARY: enc = L"bin"; break;
        case TE_ANSI: enc = L"ansi"; break;
        case TE_UTF16_LE:
        case TE_UTF16_LE_BOM: enc = L"utf16"; break;
        case TE_UTF8:
        case TE_UTF8_BOM: enc = L"utf8"; break;
    }

    Yast li_str;
    LVITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.pszText = (
        static_cast<PWSTR>(result->path) + result->path_prefix_len
        );
    lvi.iImage = SysIconIdx::file(disp_name);
    lvi.lParam = m_results.size() - 1;
    lvi.iSubItem = COL_NAME;
    lvi.iItem = m_result_list.GetItemCount();
    for (const auto& li: result->line_info)
    {
        int idx = m_result_list.InsertItem(lvi);
        if (idx < 0)
        {
            continue;
        }
        m_result_list.SetItemText(idx, COL_ENC, enc);
        li_str.format(L"%u", li.number);
        m_result_list.SetItemText(idx, COL_LINE, li_str.str());
        li_str = li.text;
        PWSTR p = static_cast<PWSTR>(li_str);
        PWSTR const end = p + li_str.length();
        while(p < end)
        {
            if (*p == '\r' || *p == '\n' || *p == '\t')
            {
                *p = L' ';
            }
            p++;
        }
        m_result_list.SetItemText(idx, COL_TEXT, li_str.str());
        lvi.iItem++;
    }

    m_result_list.SendMessage(WM_SETREDRAW, true, 0);
    RedrawWindow(
        m_result_list,
        nullptr,
        nullptr,
        RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN
        );
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::OnNext(void *pCtxt, bool was_searched)
{
    GrepDlg *self = static_cast<GrepDlg*>(pCtxt);
    self->SendMessage(WM_APP_PROGRESS, was_searched, 0);
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::OnEndSearch(void *pCtxt)
{
    GrepDlg *self = static_cast<GrepDlg*>(pCtxt);
    self->SendMessage(WM_APP_END_SEARCH, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

void GrepDlg::OnMatch(void *pCtxt, size_t num_matches, SearchResult& result)
{
    GrepDlg *self = static_cast<GrepDlg*>(pCtxt);
    self->SendMessage(WM_APP_FOUND_SEARCH, num_matches, p2lp(&result));
}

////////////////////////////////////////////////////////////////////////////////
