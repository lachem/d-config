//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#pragma once

//local
#include <config.hpp>
#include <config_node.hpp>

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
        segregate(std::forward<T>(aParams)...);
    }

    ConfigBuilder() = delete;
    ConfigBuilder(ConfigBuilder&&) = delete;
    ConfigBuilder(const ConfigBuilder&) = delete;
    ConfigBuilder& operator=(const ConfigBuilder&) = delete;

    Config build(std::vector<std::string>&& contents)
    {
        return Config(parse(std::move(contents)), separator);
    }

private:
    node_type parse(std::vector<std::string>&& contents);

    template<typename T1, typename T2, typename... T>
    void segregate(T1&& p1, T2&& p2, T&&... params)
    {
        segregate(std::forward<T1>(p1));
        segregate(std::forward<T2>(p2));
        segregate(std::forward<T >(params)...);
    }

    void segregate(const prebuild_expander_type& pre)
    {
        preexpand.push_back(pre);
    }

    void segregate(const postbuild_expander_type& post)
    {
        postexpand.push_back(post);
    }

    void segregate(const Separator& separator)
    {
        this->separator = separator;
    }

    void segregate()
    {
        //empty
    }

    std::vector<prebuild_expander_type> preexpand;
    std::vector<postbuild_expander_type> postexpand;
    Separator separator;
};

} //namespace dconfig

