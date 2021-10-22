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
#include "auto_complete_cb.h"

////////////////////////////////////////////////////////////////////////////////

AutoCompleteCombo::AutoCompleteCombo(UINT max_history) :
    MAX_HISTORY_SIZE(max_history),
    m_auto(nullptr),
    m_cur_enum_idx(0)
{
}

////////////////////////////////////////////////////////////////////////////////

HRESULT AutoCompleteCombo::QueryInterface(const IID& iid, void** ppunk)
{
    if ((iid == IID_IUnknown) || (iid == IID_IEnumString))
    {
        *ppunk = static_cast<IEnumString*>(this);
        return S_OK;
    }
    else
    {
        *ppunk = nullptr;
        return E_NOINTERFACE;
    }
}

////////////////////////////////////////////////////////////////////////////////

ULONG AutoCompleteCombo::AddRef(void)
{
    return 1;
}

////////////////////////////////////////////////////////////////////////////////

ULONG AutoCompleteCombo::Release(void)
{
    return 1;
}

////////////////////////////////////////////////////////////////////////////////

Yast AutoCompleteCombo::item_str(int idx)
{
    Yast txt;
    const int len = static_cast<int>(SendMessage(CB_GETLBTEXTLEN, idx, 0));
    if (len != CB_ERR)
    {
        txt.clear(len);
        SendMessage(CB_GETLBTEXT, idx, txt);
    }
    return txt;
}

////////////////////////////////////////////////////////////////////////////////

int AutoCompleteCombo::item_count()
{
    return static_cast<int>(SendMessage(CB_GETCOUNT, 0, 0));
}

////////////////////////////////////////////////////////////////////////////////

HRESULT AutoCompleteCombo::Next(
    ULONG items_requested,
    PWSTR* dest_items,
    ULONG* p_incremented_cnt
    )
{
    const UINT vsize = item_count();
    UINT i = 0;
    for ( ; i < items_requested; i++)
    {
        if (m_cur_enum_idx == vsize)
        {
            break;
        }

        Yast txt(item_str(m_cur_enum_idx));
        UINT bytes = txt.byte_length() + sizeof(WCHAR);
        PWSTR cpy = static_cast<PWSTR>(CoTaskMemAlloc(bytes));
        memcpy(cpy, txt.str(), bytes);
        dest_items[i] = cpy;

        m_cur_enum_idx++;
        if (p_incremented_cnt)
        {
            (*p_incremented_cnt)++;
        }
    }

    return i == items_requested ? S_OK : S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT AutoCompleteCombo::Skip(ULONG num_items)
{
    const UINT vsize = item_count();
    if ((m_cur_enum_idx + num_items) >= vsize)
    {
        return S_FALSE;
    }
    m_cur_enum_idx += num_items;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT AutoCompleteCombo::Reset(void)
{
    m_cur_enum_idx = 0;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT AutoCompleteCombo::Clone(IEnumString** ppenum)
{
    UNUSED(ppenum);
    return E_OUTOFMEMORY;
}

////////////////////////////////////////////////////////////////////////////////

AutoCompleteCombo::~AutoCompleteCombo()
{
    if (m_auto) m_auto->Release();
}

////////////////////////////////////////////////////////////////////////////////

bool AutoCompleteCombo::connect(HWND hCombo)
{
    const UINT required = CBS_DROPDOWN;
    const UINT forbidden = CBS_SIMPLE | CBS_SORT;
    m_hWnd = hCombo;
    DWORD style = GetStyle();
    if ((style & required) != required || (style & forbidden) != 0)
    {
        return false;
    }

    COMBOBOXINFO cbi = { sizeof(cbi) };
    SendMessage(CB_GETCOMBOBOXINFO, 0, p2lp(&cbi));
    m_edit = cbi.hwndItem;
    return (m_hWnd != nullptr && m_edit != nullptr);
}

////////////////////////////////////////////////////////////////////////////////

bool AutoCompleteCombo::enable(bool enable)
{
    if (m_hWnd == nullptr || m_edit == nullptr)
    {
        return false;
    }

    if (m_auto)
    {
        m_auto->Release();
        m_auto = nullptr;
    }

    HRESULT hr = S_OK;
    if (enable)
    {
        hr = CoCreateInstance(
            CLSID_AutoComplete,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_auto)
            );
        if (SUCCEEDED(hr))
        {
            hr = m_auto->Init(m_edit, this, nullptr, nullptr);
            if (SUCCEEDED(hr))
            {
                m_auto->SetOptions(ACO_AUTOSUGGEST);
                m_auto->Enable(true);
            }
        }
    }
    return SUCCEEDED(hr);
}

////////////////////////////////////////////////////////////////////////////////

void AutoCompleteCombo::add_entry(const Yast& text)
{
    if (text.is_empty())
    {
        return;
    }

    if (!(m_subkey.is_empty() || m_value_prefix.is_empty()))
    {
        load(m_subkey, m_value_prefix);
    }

    remove_entry(text);
    SendMessage(CB_INSERTSTRING, 0, p2lp(text.str()));
    SetText(text);
    m_cur_enum_idx = 0;
}

////////////////////////////////////////////////////////////////////////////////

void AutoCompleteCombo::remove_entry(PCWSTR str)
{
    Yast txt(str);

    LRESULT idx = SendMessage(CB_FINDSTRINGEXACT, ~0U, txt);
    while (idx != CB_ERR)
    {
        SendMessage(CB_DELETESTRING, idx, 0);
        idx = SendMessage(CB_FINDSTRINGEXACT, ~0U, txt);
    }
}

////////////////////////////////////////////////////////////////////////////////

void AutoCompleteCombo::remove_selected()
{
    UINT idx = static_cast<UINT>(SendMessage(CB_GETCURSEL, 0, 0));
    if (idx != CB_ERR)
    {
        SendMessage(CB_DELETESTRING, idx, 0);
        m_cur_enum_idx = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////

int AutoCompleteCombo::load(PCWSTR subkey, PCWSTR prefix)
{
    if (subkey == nullptr || prefix == nullptr || *subkey == 0)
    {
        return -1;
    }

    m_subkey = subkey;
    m_value_prefix = prefix;

    RegAutoKey rkey (m_subkey);
    if (!rkey)
    {
        return -1;
    }
    SendMessage(CB_RESETCONTENT, 0, 0);

    Yast name, value;
    for (UINT i = 0; i < MAX_HISTORY_SIZE; i++)
    {
        name.format(L"%s%u", prefix, i);
        if (ReadRegString(rkey, name, value))
        {
            SendMessage(CB_INSERTSTRING, ~0U, value);
        }
    }
    return item_count();
}

////////////////////////////////////////////////////////////////////////////////

void AutoCompleteCombo::save()
{
    if (m_subkey.is_empty() || m_value_prefix.is_empty())
    {
        return;
    }
    RegAutoKey rkey (m_subkey);
    if (!rkey)
    {
        return;
    }

    UINT vsize = item_count();
    UINT num_save = vsize <= MAX_HISTORY_SIZE ? vsize : MAX_HISTORY_SIZE;
    Yast name;
    for (UINT i = 0; i < MAX_HISTORY_SIZE; i++)
    {
        name.format(L"%s%u", m_value_prefix.str(), i);
        if (i < num_save)
        {
            WriteRegString(rkey, name, item_str(i));
        }
        else
        {
            RegDeleteValue(rkey, name);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void AutoCompleteCombo::scale_item_height(UINT numerator, UINT denominator)
{
    const LRESULT height = SendMessage(CB_GETITEMHEIGHT, 0, 0);
    SendMessage(CB_SETITEMHEIGHT, 0, (height * numerator) / denominator);
}

////////////////////////////////////////////////////////////////////////////////

Yast AutoCompleteCombo::GetText()
{
    const UINT idx = static_cast<UINT>(SendMessage(CB_GETCURSEL, 0, 0));
    return (idx != CB_ERR) ? item_str(idx) : Yast(m_hWnd);
}

////////////////////////////////////////////////////////////////////////////////
