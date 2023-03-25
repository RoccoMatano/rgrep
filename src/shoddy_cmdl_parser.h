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

////////////////////////////////////////////////////////////////////////////////
//
// This shoddy command line parser only supports optional arguments (as opposed
// to positional arguments). All of these have to be given as key/value pairs.
// They have to be in the form of <[-/]key>[:= ]<value>, eg. '-key=value' or
// '/abc:xyz' or '-uvw 123'. An argument may or may not have a value. If the key
// or the value contain spaces and ':' or '=' are used as seperator, the whole
// argument has to be quoted, eg. '"/a c:x z"'. If ' ' is used as seperator,
// key and value have to be quoted separately, e.g. '"/a c" "x z"'.

class ShoddyCmdlParser
{
private:
    cmap<Yast, Yast> m_map;

    void insert_or_assign(const Yast& key, const Yast& value)
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            it->second = value;
        }
        else
        {
            m_map.insert({key, value});
        }
    }

    ////////////////////////////////////////////////////////////////////////////

public:
    ShoddyCmdlParser(int argc, PWSTR *argv)
    {
        Yast last_key;
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '/' || argv[i][0] == '-')
            {
                last_key.clear();
                if (!argv[i][1])
                {
                    continue;
                }
                Yast arg(&argv[i][1]);
                int idx = arg.find(1, L":");
                int idx_equal = arg.find(1, L"=");
                if (idx < 0)
                {
                    idx = idx_equal;
                }
                else
                {
                    if (idx_equal >= 0 && idx_equal < idx)
                    {
                        idx = idx_equal;
                    }
                }
                if (idx >= 0)
                {
                    insert_or_assign(
                        arg.slice(0, idx),
                        arg.slice(idx + 1, -1)
                        );
                }
                else
                {
                    insert_or_assign(arg, Yast());
                    last_key = arg;
                }
            }
            else
            {
                if (!last_key.is_empty())
                {
                    insert_or_assign(last_key, Yast(argv[i]));
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    bool has_key(PCWSTR key)
    {
        const auto it = m_map.find(key);
        return it != m_map.end();
    }

    ////////////////////////////////////////////////////////////////////////////

    Yast get_val(PCWSTR key)
    {
        const auto it = m_map.find(key);
        return it != m_map.end() ? it->second : Yast();
    }
};

////////////////////////////////////////////////////////////////////////////////

