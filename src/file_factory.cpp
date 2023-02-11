//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include <file_factory.hpp>
#include <default_factory.hpp>

//std
#include <fstream>

namespace dconfig {

Config FileFactory::create() const
{
    auto contents = std::vector<std::string>{};
    for(auto&& filename : files)
    {
        if (auto in = std::ifstream{filename, std::ios::in | std::ios::binary})
        {
            std::string loaded;
            in.seekg(0, std::ios::end);
            loaded.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&loaded[0], loaded.size());
            in.close();
            contents.push_back(std::move(loaded));
        }
    }
    return DefaultFactory(separator).create(std::move(contents));
}

} //namespace dconfig

