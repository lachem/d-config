//          Copyright Adam Lach 2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <default_factory.hpp>

namespace dconfig {

struct [[deprecated("Use DefaultFactory instead")]] DefaultBuilder : public DefaultFactory
{
    using DefaultFactory::DefaultFactory;

    [[deprecated]] [[nodiscard]] Config build(const std::vector<std::string>& contents) const;
    [[deprecated]] [[nodiscard]] Config build(std::vector<std::string>&& contents) const;
};

} //namespace dconfig

