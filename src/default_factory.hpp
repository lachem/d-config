//          Copyright Adam Lach 2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <config.hpp>
#include <separator.hpp>
#include <array_key.hpp>

//std
#include <string>
#include <vector>

namespace dconfig {

struct DefaultFactory
{
    explicit DefaultFactory(Separator separator = {}, ArrayKey arrayKey = {})
        : separator(separator)
        , arrayKey(arrayKey)
    {
    }

    DefaultFactory(DefaultFactory&&) = delete;
    DefaultFactory(const DefaultFactory&) = delete;
    DefaultFactory& operator=(const DefaultFactory&) = delete;

    [[nodiscard]] Config create(const std::vector<std::string>& contents) const;
    [[nodiscard]] Config create(std::vector<std::string>&& contents) const;

private:
    Separator separator;
    ArrayKey arrayKey;
};

} //namespace dconfig

