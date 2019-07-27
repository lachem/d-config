//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <config.hpp>
#include <file_factory.hpp>
#include <separator.hpp>
#include <array_key.hpp>

//std
#include <string>
#include <vector>

namespace dconfig {

struct InitFactory
{
    explicit InitFactory(int argc, char** argv, Separator separator = Separator(), ArrayKey arrayKey = ArrayKey())
        : argc(argc)
        , argv(argv)
        , separator(separator)
        , arrayKey(arrayKey)
    {
    }

    Config create() const;

private:
    std::vector<std::string> readFiles() const;

    int argc;
    char** argv;
    Separator separator;
    ArrayKey arrayKey;
};


} //namespace dconfig

