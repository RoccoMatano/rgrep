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

#include "pch.h"
#include "rgrep_rx.h"
#include "pcre2_16/pcre2.h"

// We are going to pass PWSTR to PCRE, so it be better configured for
// a code unit width of 16.
static_assert(PCRE2_CODE_UNIT_WIDTH == 16, "unexpected width");
static_assert(sizeof(PCRE2_UCHAR) == sizeof(WCHAR), "unexpected size");

////////////////////////////////////////////////////////////////////////////////

class rrx::pimpl
{
public:
    pcre2_code *m_code;
    pcre2_match_data *m_match;

    pimpl() : m_code(nullptr), m_match(nullptr)
    {
    }

    ~pimpl()
    {
        if (m_code) pcre2_code_free(m_code);
        if (m_match) pcre2_match_data_free(m_match);
    }

    bool compile(const Yast& regex, UINT flags);
    bool search(range& found, const Yast& subject, size_t offset) const;
    Yast replace(const Yast& subject, const Yast& replacement) const;
};

////////////////////////////////////////////////////////////////////////////////

bool rrx::pimpl::compile(const Yast& regex, UINT flags)
{
    // While most of the options could be transfered to PCRE as options bits,
    // WHOLE_WORDS cannot be indicated this way. For WHOLE_WORDS we have to
    // embed the regex between '\b' markers: '\b<regex>\b'. But if we are
    // doing that, we cannot use the PCRE flag PCRE2_LITERAL anymore, since
    // that would remove the special meaning from '\b'. So we have to implement
    // our LITERAL flag by embedding the regex between '\Q' and '\E' markers.
    // That means we have to handle the LITERAL flag first and then the
    // WHOLE_WORDS flag by surrounding the given regex with additional
    // characters.

    uint32_t options = PCRE2_UTF | PCRE2_NO_UTF_CHECK;
    if (flags & IGNORE_CASE) options |= PCRE2_CASELESS;
    if (flags & DOT_ALL) options |= PCRE2_DOTALL;
    if (flags & MULTI_LINE) options |= PCRE2_MULTILINE;

    Yast actual_rx(regex);
    if (flags & LITERAL)
    {
        actual_rx = L"\\Q" + actual_rx.replace(L"\\E", L"\\\\E") + L"\\E";
    }
    if (flags & WHOLE_WORDS)
    {
        actual_rx = L"\\b" + actual_rx + L"\\b";
    }

    int error_code;
    size_t error_offset;
    pcre2_code *code = pcre2_compile(
        actual_rx,
        actual_rx.length(),
        options,
        &error_code,
        &error_offset,
        nullptr
        );
    if (code)
    {
        m_code = code;
        m_match = pcre2_match_data_create_from_pattern(m_code, nullptr);
        return true;
    }
    else
    {
        TRACE("rrx: '%S' does not compile!\n", actual_rx.str());
        TRACE("options: %08x\n", options);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool rrx::pimpl::search(range& found, const Yast& subject, size_t offset) const
{
    const int res = pcre2_match(
        m_code,
        subject.str(),
        subject.length(),
        offset,
        0,
        m_match,
        nullptr
        );
    const bool ok = res >= 1;
    if (ok)
    {
        size_t *ov = pcre2_get_ovector_pointer(m_match);
        found.begin = ov[0];
        found.end = ov[1];
    }
    return ok;
}

////////////////////////////////////////////////////////////////////////////////

Yast rrx::pimpl::replace(const Yast& subject, const Yast& replacement) const
{
    const uint32_t opt = (
        PCRE2_NO_UTF_CHECK |                // do not check 'replacement'
        PCRE2_SUBSTITUTE_GLOBAL |           // replace more than once
        PCRE2_SUBSTITUTE_OVERFLOW_LENGTH    // calculate required length
        );
    const size_t sub_len = subject.length();
    const size_t rep_len = replacement.length();

    // How much space will be needed for the result? We cannot know that in
    // advance. So we try with a buffer that is 150% of the source. If that
    // was not big enough, we are told how much is needed and repeat with
    // that size.
    size_t out_size = (sub_len * 3) / 2;
    PWSTR out = nullptr;

    int res, cnt = 0;
    do
    {
        free(out);
        out = p2p<PWSTR>(malloc(out_size * sizeof(WCHAR)));
        res = pcre2_substitute(
            m_code,
            subject,
            sub_len,
            0,
            opt,
            m_match,
            nullptr,
            replacement,
            rep_len,
            out,
            &out_size
            );
    } while (res == PCRE2_ERROR_NOMEMORY && ++cnt < 2);

    UINT ysize = 0;
    PWSTR ysrc = nullptr;
    if (res >= 0)
    {
        ysize = static_cast<UINT>(out_size);
        ysrc = out;
    }
    Yast replaced(ysrc, ysize);
    free(out);
    return replaced;
}

////////////////////////////////////////////////////////////////////////////////

rrx::rrx() : m_pimpl(new pimpl())
{
}

////////////////////////////////////////////////////////////////////////////////

rrx::~rrx()
{
    delete m_pimpl;
}

////////////////////////////////////////////////////////////////////////////////

rrx::ptr rrx::compile(const Yast& regex, UINT flags)
{
    rrx *self = new rrx();
    if (!self->m_pimpl->compile(regex, flags))
    {
        delete self;
        self = nullptr;
    }
    return ptr(self);
}

////////////////////////////////////////////////////////////////////////////////

bool rrx::search(range& found, const Yast& subject, size_t offset) const
{
    return m_pimpl->search(found, subject, offset);
}

////////////////////////////////////////////////////////////////////////////////

Yast rrx::replace(const Yast& subject, const Yast& replacement) const
{
    return m_pimpl->replace(subject, replacement);
}

////////////////////////////////////////////////////////////////////////////////
