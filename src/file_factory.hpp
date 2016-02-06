//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include "config.hpp"

//std
#include <string>
#include <vector>

namespace dconfig 
{

struct FileFactory
{
    typedef Config::separator_type separator_type;

    FileFactory(const std::vector<std::string>& aFileList, const separator_type& aSeparator = separator_type());

    Config create() const
    {
        return Config(readFiles(files), separator);
    }

private:
    std::vector<std::string> readFiles(const std::vector<std::string>& files) const;

    std::vector<std::string> files;
    Config::separator_type separator;
};

} //namespace dconfig

