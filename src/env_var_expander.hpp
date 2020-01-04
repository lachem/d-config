//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <separator.hpp>

//boost
#include <boost/xpressive/xpressive.hpp>

//std
#include <string>

namespace dconfig {

/// Fulfills prebuild expander concept
struct EnvVarExpander
{
private:
    struct RegexExpander
    {
        std::string operator()(const boost::xpressive::smatch& what) const
        {
            assert(what.size()>1);

            const char* env = getenv((++what.begin())->str().c_str());
            return env ? env : what.str();
        }
    };

public:
    void operator()(std::string& contents) const
    {
        using namespace boost::xpressive;

        sregex env = "%env." >> (s1 = -+_) >> "%";
        auto it = regex_replace(contents.begin(), contents.begin(), contents.end(), env, RegexExpander());

        //clear out the spare part of the string to avoid reallaoc inside contents
        for (; it != contents.end(); ++it)
        {
            *it = ' ';
        }
    }
};

} //namespace dconfig

