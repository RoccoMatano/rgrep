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

#include "res/resource.h"
#include "font_size_dlg.h"

////////////////////////////////////////////////////////////////////////////////

class SettingsDlg : public DpiScaledDlg
{
public:
    SettingsDlg(BaseWnd* Parent, Yast& edit_cmd, Yast& view_cmd, Yast& csv_sep):
        DpiScaledDlg(Parent),
        m_edit_cmd(edit_cmd),
        m_view_cmd(view_cmd),
        m_csv_sep(csv_sep)
    {
    }

    INT_PTR DoModal()
    {
        return DpiScaledDlg::DoModal(IDD_SETTINGS);
    }

    Yast get_edit_cmd()
    {
        return m_edit_cmd;
    }

    Yast get_view_cmd()
    {
        return m_view_cmd;
    }

    Yast get_csv_sep()
    {
        return m_csv_sep;
    }

protected:
    Yast m_edit_cmd;
    Yast m_view_cmd;
    Yast m_csv_sep;
    bool OnInitDialog() override;
    bool OnCommand(UINT CmdId, UINT Notification, HWND Ctrl) override;
};

////////////////////////////////////////////////////////////////////////////////
