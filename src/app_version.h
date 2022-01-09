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

#pragma once

// please keep the following 4 defines up-to-date
#define APP_VERSION_HI          0
#define APP_VERSION_LO          11
#define APP_REVISION            17
#define APP_BUILD_NUMBER        69

//////////////////////////////////////////////////////////////////////////////

#define AV_STR1(x)          #x
#define AV_STR(x)           AV_STR1(x)

#define APP_STRING_BASE AV_STR(APP_VERSION_HI) "." \
                        AV_STR(APP_VERSION_LO) "." \
                        AV_STR(APP_REVISION) "." \
                        AV_STR(APP_BUILD_NUMBER)

//////////////////////////////////////////////////////////////////////////////

#define APP_VERSION_STRING  APP_STRING_BASE

#define COMPANY_NAME    "RoMa"
#define COPYRIGHT       L"\251 RoMa 2018-2022"
#define PRODUCT_NAME    "rgrep"
