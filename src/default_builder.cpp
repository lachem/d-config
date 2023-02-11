//          Copyright Adam Lach 2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <default_builder.hpp>

namespace dconfig {

Config DefaultBuilder::build(const std::vector<std::string>& contents) const
{
    return create(contents);
}

Config DefaultBuilder::build(std::vector<std::string>&& contents) const
{
    return create(std::move(contents));
}

} //namespace dconfig

