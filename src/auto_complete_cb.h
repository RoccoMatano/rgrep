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

class AutoCompleteCombo : public BaseWnd, IEnumString
{
public:
    AutoCompleteCombo(UINT max_history = 20);
    ~AutoCompleteCombo();

    // use in this order
    bool connect(HWND hCombo);
    int load(PCWSTR subkey, PCWSTR value_prefix);
    bool enable(bool enable);

    BaseWnd get_edit()
    {
        return m_edit;
    }
    void save();
    void add_entry(const Yast& text);
    void remove_entry(PCWSTR str);
    void remove_selected();
    void scale_item_height(UINT numerator, UINT denominator);
    int item_count();
    Yast item_str(int idx);

    // overriding BaseWnd::GetText
    Yast GetText();

protected:
    const UINT MAX_HISTORY_SIZE;
    Yast m_subkey;
    Yast m_value_prefix;
    IAutoComplete2 *m_auto;
    BaseWnd m_edit;
    BaseWnd m_list;
    UINT m_cur_enum_idx;

    // IUnknown methods
    IFACEMETHODIMP QueryInterface(const IID& iid, void** ppunk);
    IFACEMETHODIMP_(ULONG) AddRef(void);
    IFACEMETHODIMP_(ULONG) Release(void);

    // IEnumString methods
    IFACEMETHODIMP Next(
        ULONG items_requested,
        PWSTR* dest_items,
        ULONG* p_incremented_cnt
        );
    IFACEMETHODIMP Skip(ULONG num_items);
    IFACEMETHODIMP Reset(void);
    IFACEMETHODIMP Clone(IEnumString** ppenum);
};
