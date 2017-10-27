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
namespace detail {

// TODO: ConfigRoot is now a builder and should be moved to separate compilation unit
// TODO: Add parameters for prebuild and postbuild expanders (executed in provided sequence)
struct ConfigRoot
{
    struct Separator
    {
        Separator() : value(default_value()) {}
        Separator(char value) : value(value) {}
        Separator(const Separator&) = default;

        static char default_value() { return '.'; }

        char value;
    };

    using node_type = Node;

    explicit ConfigRoot(const std::vector<std::string>& contents, Separator aSeparator = Separator())
        : separator(aSeparator)
    {
        parse(contents);
    }

    ConfigRoot(ConfigRoot&&) = delete;
    ConfigRoot(const ConfigRoot&) = delete;
    ConfigRoot& operator=(const ConfigRoot&) = delete;

    const node_type& getNode() const { return node; }

private:
    void parse(const std::vector<std::string>& contents);

    Separator separator;
    node_type node;
};

} //namespace detail

struct Config
{
    typedef detail::ConfigRoot::Separator separator_type;
    typedef detail::ConfigRoot::node_type node_type;

    explicit Config(const std::vector<std::string>& aFileList, const separator_type& aSeparator = separator_type())
        : root(new detail::ConfigRoot(aFileList, aSeparator))
        , separator(aSeparator)
        , node(&root->getNode())
    {
    }

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
            ? node->getValues(path, separator.value)
            : empty;
    }

    Config scope(const std::string& path) const
    {
        if (node)
        {
            const auto& nodes = node->getNodes(path, separator.value);
            if (!nodes.empty())
            {
                return Config(root, nodes[0].get(), separator);
            }
        }
        return Config(root, nullptr, separator);
    }

    std::vector<Config> scopes(const std::string& path) const
    {
        std::vector<Config> result;
        if(node)
        {
            const auto& nodes = node->getNodes(path, separator.value);
            for (const auto& sub : nodes)
            {
                result.emplace_back(Config(root, sub.get(), separator));
            }
        }
        return result;
    }

    explicit operator bool() const noexcept
    {
        return node;
    }

private:
    Config(std::shared_ptr<detail::ConfigRoot> aConfigRoot
         , const node_type* aNode
         , const separator_type& aSeparator)
        : root(aConfigRoot)
        , separator(aSeparator)
        , node(aNode)
    {}

    std::shared_ptr<detail::ConfigRoot> root;
    separator_type separator;
    const node_type* node;
};

template<>
inline boost::optional<std::string> Config::get<std::string>(const std::string& path) const
{
    if(node)
    {
        const auto& values = node->getValues(path, separator.value);
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
        ? node->getValues(path, separator.value)
        : empty;
}

} //namespace dconfig

