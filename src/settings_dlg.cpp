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
#include "settings_dlg.h"

////////////////////////////////////////////////////////////////////////////////

bool SettingsDlg::OnInitDialog()
{
    BaseWnd Ctrl(GetItem(IDC_EDITOR_CMD));
    Ctrl.SetText(m_edit_cmd);
    Ctrl = GetItem(IDC_VIEWER_CMD);
    Ctrl.SetText(m_view_cmd);
    Ctrl = GetItem(IDC_CSV_SEP);
    Ctrl.SetText(m_csv_sep);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool SettingsDlg::OnCommand(UINT CmdId, UINT Notification, HWND Ctrl)
{
    if (CmdId == IDCANCEL)
    {
        EndDialog(m_hWnd, CmdId);
    }
    else if (CmdId == IDOK)
    {
        m_edit_cmd = Yast(GetItem(IDC_EDITOR_CMD));
        m_view_cmd = Yast(GetItem(IDC_VIEWER_CMD));
        m_csv_sep = Yast(GetItem(IDC_CSV_SEP));
        EndDialog(m_hWnd, CmdId);
    }
    else if (
        CmdId == IDC_SEARCH_PATH_BROWSE_EDIT ||
        CmdId == IDC_SEARCH_PATH_BROWSE_VIEW
        )
    {
        static const COMDLG_FILTERSPEC filters[] =
        {
            {L"Programs", L"*.exe"},
            {L"All Files",  L"*.*"}
        };

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
                FOS_FILEMUSTEXIST |
                FOS_DONTADDTORECENT
                );
            pfd->SetFileTypes(ARRAYSIZE(filters), filters);

            if (SUCCEEDED(pfd->Show(m_hWnd)))
            {
                IShellItem *item = nullptr;
                pfd->GetResult(&item);
                if (item)
                {
                    PWSTR pFilePath = nullptr;
                    item->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath);
                    const UINT id = (
                        CmdId == IDC_SEARCH_PATH_BROWSE_EDIT ?
                        IDC_EDITOR_CMD :
                        IDC_VIEWER_CMD
                        );
                    GetItem(id).SetText(pFilePath);
                    CoTaskMemFree(pFilePath);
                    item->Release();
                }
            }
            pfd->Release();
        }
    }
    return true;
    UNUSED(Notification);
    UNUSED(Ctrl);
}

////////////////////////////////////////////////////////////////////////////////
