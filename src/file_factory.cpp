//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include "file_factory.hpp"

//std
#include <fstream>

namespace dconfig 
{

std::vector<std::string> FileFactory::readFiles(const std::vector<std::string>& files) const
{
    std::vector<std::string> contents;
    for(auto&& filename : files)
    {
        std::ifstream fileStream(filename);
        contents.push_back(std::string(
            std::istreambuf_iterator<char>(fileStream),
            std::istreambuf_iterator<char>())
        );
    }
    return std::move(contents);
}

} //namespace dconfig

