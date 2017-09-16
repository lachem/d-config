//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//boost
#include <boost/xpressive/xpressive_fwd.hpp>

//std
#include <string>

namespace dconfig {

    /// Fulfills prebuild expander expander conept
    struct EnvVarExpander
    {
        std::string operator()(const std::string&) const;
        std::string operator()(const boost::xpressive::smatch&) const;
    };

} //namespace dconfig

