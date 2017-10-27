//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <config.hpp>
#include <separator.hpp>

//std
#include <string>
#include <vector>

namespace dconfig
{

struct FileFactory
{
    template<typename T>
    explicit FileFactory(T&& aFileList, const Separator& aSeparator = '.')
        : files(aFileList)
        , separator(aSeparator)
    {
    }

    Config create() const
    {
        return Config(readFiles(files), separator);
    }

private:
    std::vector<std::string> readFiles(const std::vector<std::string>& files) const;

    std::vector<std::string> files;
    Separator separator;
};

} //namespace dconfig

