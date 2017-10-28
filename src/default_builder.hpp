//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#pragma once

//local
#include <config.hpp>

//std
#include <string>
#include <vector>

namespace dconfig {

struct DefaultBuilder
{
    explicit DefaultBuilder(Separator aSeparator = Separator())
        : separator(aSeparator)
    {
    }

    DefaultBuilder(DefaultBuilder&&) = delete;
    DefaultBuilder(const DefaultBuilder&) = delete;
    DefaultBuilder& operator=(const DefaultBuilder&) = delete;

    Config build(const std::vector<std::string>& contents) const;
    Config build(std::vector<std::string>&& contents) const;

private:
    Separator separator;
};

} //namespace dconfig

