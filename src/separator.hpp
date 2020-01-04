//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace dconfig {

struct Separator
{
    Separator() = default;
    explicit Separator(char aValue) : value(aValue) {}

    operator char () const { return value; }
    char value = '.';
};

} //namespace dconfig
