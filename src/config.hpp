//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#pragma once

//local
#include <node.hpp>
#include <separator.hpp>

//boost
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

//std
#include <map>
#include <memory>

namespace dconfig {

class Config
{
public:
    template<typename T>
    boost::optional<T> get(const std::string& path) const
    {
        if(node)
        {
            auto value = get<std::string>(path);
            if (value)
            {
                return boost::lexical_cast<T>(*value);
            }
        }
        return boost::optional<T>();
    }

    template<typename T>
    std::vector<T> getAll(const std::string& path) const
    {
        std::vector<T> result;
        if(node)
        {
            auto&& values = getAll<std::string>(path);
            for (const auto& value : values)
            {
                result.emplace_back(boost::lexical_cast<T>(value));
            }
        }

        return result;
    }

    const std::vector<std::string>& getRef(const std::string& path) const
    {
        static const std::vector<std::string> empty;
        return (node)
            ? node->getValues(path, separator)
            : empty;
    }

    Config scope(const std::string& path) const
    {
        if (node)
        {
            const auto& nodes = node->getNodes(path, separator);
            if (!nodes.empty())
            {
                return Config(nodes[0], separator);
            }
        }
        return Config(nullptr, separator);
    }

    std::vector<Config> scopes(const std::string& path) const
    {
        std::vector<Config> result;
        if(node)
        {
            const auto& nodes = node->getNodes(path, separator);
            for (const auto& sub : nodes)
            {
                result.emplace_back(Config(sub, separator));
            }
        }
        return result;
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(node);
    }

private:
    friend class ConfigBuilder;

    Config(const detail::Node::node_type& aNode, const Separator& aSeparator)
        : separator(aSeparator)
        , node(aNode)
    {
    }

    Separator separator;
    detail::Node::node_type node;
};

template<>
inline boost::optional<std::string> Config::get<std::string>(const std::string& path) const
{
    if(node)
    {
        const auto& values = node->getValues(path, separator);
        if (!values.empty())
        {
            return boost::optional<std::string>(values[0]);
        }
    }
    return boost::optional<std::string>();
}

template<>
inline std::vector<std::string> Config::getAll<std::string>(const std::string& path) const
{
    static std::vector<std::string> empty;
    return (node)
        ? node->getValues(path, separator)
        : empty;
}

} //namespace dconfig

