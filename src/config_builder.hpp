//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <config.hpp>
#include <config_node.hpp>
#include <separator.hpp>
#include <array_key.hpp>

//std
#include <memory>
#include <functional>
#include <vector>

namespace dconfig {

struct ConfigBuilder
{
    using node_type = detail::ConfigNode::node_type;
    using prebuild_expander_type = std::function<void (std::string&)>;
    using postbuild_expander_type = std::function<void (detail::ConfigNode&)>;

    template<typename... T>
    explicit ConfigBuilder(T&&... aParams)
    {
        (segregate(std::forward<T>(aParams)), ...);
    }

    ConfigBuilder() = delete;
    ConfigBuilder(ConfigBuilder&&) = delete;
    ConfigBuilder(const ConfigBuilder&) = delete;
    ConfigBuilder& operator=(const ConfigBuilder&) = delete;

    ConfigBuilder& addPreExpander(const prebuild_expander_type& expander)
    {
        preexpand.push_back(expander);
        return *this;
    }

    ConfigBuilder& addPostExpander(const postbuild_expander_type& expander)
    {
        postexpand.push_back(expander);
        return *this;
    }

    Config build(std::vector<std::string>&& contents)
    {
        return Config(parse(std::move(contents)), separator);
    }

private:
    node_type parse(std::vector<std::string>&& contents);

    void segregate(Separator separator)
    {
        this->separator = separator;
    }

    void segregate(ArrayKey arrayKey)
    {
        this->arrayKey = arrayKey;
    }

    void segregate()
    {
        //empty
    }

    std::vector<prebuild_expander_type> preexpand;
    std::vector<postbuild_expander_type> postexpand;
    Separator separator;
    ArrayKey arrayKey;
};

} //namespace dconfig

