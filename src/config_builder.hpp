//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#pragma once

//local
#include <node.hpp>

//boost
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

//std
#include <map>
#include <memory>

namespace dconfig {

// TODO: Add parameters for prebuild and postbuild expanders (executed in provided sequence)
struct ConfigBuilder
{
    using node_type = detail::Node;

    explicit ConfigBuilder(const std::vector<std::string>& aContents, Separator aSeparator = '.')
        : separator(aSeparator)
    {
        parse(aContents);
    }

    ConfigBuilder(ConfigBuilder&&) = delete;
    ConfigBuilder(const ConfigBuilder&) = delete;
    ConfigBuilder& operator=(const ConfigBuilder&) = delete;

    const node_type& getNode() const { return node; }

private:
    void parse(const std::vector<std::string>& contents);

    Separator separator;
    node_type node;
};

} //namespace dconfig

