//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include "config.hpp"
#include "file_factory.hpp"

namespace dconfig 
{

struct InitFactory
{
    typedef Config::separator_type separator_type;

    InitFactory(int argc, char** argv, const separator_type& aSeparator = separator_type());

    Config create() const;

private:
    std::vector<std::string> readFiles() const;

    int argc;
    char** argv;
    Config::separator_type separator;
};


} //namespace dconfig

