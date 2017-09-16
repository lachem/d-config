//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//boost
#include <boost/xpressive/xpressive.hpp>

//std
#include <string>

namespace dconfig {

/// Fulfills prebuild expander expander conept
struct EnvVarExpander
{
    void operator()(std::string& contents) const
    {
        using namespace boost::xpressive;

        sregex env = "%env." >> (s1 = -+_) >> "%";
        contents = std::move(regex_replace(contents, env, *this));
    }

    std::string operator()(const boost::xpressive::smatch& what) const
    {
        assert(what.size()>1);

        const char* env = getenv((++what.begin())->str().c_str());
        return env ? env : what.str();
    }
};

} //namespace dconfig

