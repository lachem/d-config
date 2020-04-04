//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <config_node.hpp>
#include <separator.hpp>

//boost
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/type_index.hpp>

//std
#include <vector>
#include <exception>

namespace dconfig {

class Config
{
public:
    template<typename T, typename P1, typename P2, typename... P>
    boost::optional<T> get(P1&& p1, P2&& p2, P&&... p) const
    {
        if (auto&& result = this->get<T>(p1))
            return result;
        return this->get<T>(std::forward<P2>(p2), std::forward<P>(p)...);
    }

    template<typename T>
    boost::optional<T> get(const std::string& path) const
    {
        return this->get<T>(path.c_str());
    }

    template<typename T>
    boost::optional<T> get(const char* path) const
    {
        if(node)
        {
            auto value = get<std::string>(path);
            if (value)
            {
                try
                {
                    return boost::lexical_cast<T>(*value);
                }
                catch(boost::bad_lexical_cast& ex)
                {
                    throw std::invalid_argument(conversionError<T>(*value, std::string(path)));
                }
            }
        }
        return boost::optional<T>();
    }

    template<typename T, typename P1, typename P2, typename... P>
    std::vector<T> getAll(P1&& p1, P2&& p2, P&&... p) const
    {
        auto&& result = this->getAll<T>(p1);
        if (!result.empty())
            return result;
        return this->getAll<T>(std::forward<P2>(p2), std::forward<P>(p)...);
    }

    template<typename T>
    std::vector<T> getAll(const std::string& path) const
    {
        return this->getAll<T>(path.c_str());
    }

    template<typename T>
    std::vector<T> getAll(const char* path) const
    {
        std::vector<T> result;
        if(node)
        {
            size_t index = 0;
            for (const auto& value : getAll<std::string>(path))
            {
                try
                {
                    result.push_back(boost::lexical_cast<T>(value));
                    ++index;
                }
                catch(boost::bad_lexical_cast& ex)
                {
                    auto&& indexedPath = std::string(path) + "[" + std::to_string(index) + "]";
                    throw std::invalid_argument(conversionError<T>(value, std::move(indexedPath)));
                }
            }
        }

        return result;
    }

    template<typename P1, typename P2, typename... P>
    const std::vector<std::string>& getRef(P1&& p1, P2&& p2, P&&... p) const
    {
        const auto& result = this->getRef(p1);
        if (!result.empty())
            return result;
        return this->getRef(std::forward<P2>(p2), std::forward<P>(p)...);
    }

    const std::vector<std::string>& getRef(const std::string& path) const
    {
        return this->getRef(path.c_str());
    }

    const std::vector<std::string>& getRef(const char* path) const
    {
        static const std::vector<std::string> empty;
        return (node)
            ? node->getValues(path, separator)
            : empty;
    }

    template<typename P1, typename P2, typename... P>
    Config scope(P1&& p1, P2&& p2, P&&... p) const
    {
        if (auto&& result = this->scope(p1))
            return result;
        return this->scope(std::forward<P2>(p2), std::forward<P>(p)...);
    }

    Config scope(const std::string& path) const
    {
        return this->scope(path.c_str());
    }

    Config scope(const char* path) const
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

    template<typename P1, typename P2, typename... P>
    std::vector<Config> scopes(P1&& p1, P2&& p2, P&&... p) const
    {
        auto&& result = this->scopes(p1);
        if (!result.empty())
            return result;
        return this->scopes(std::forward<P2>(p2), std::forward<P>(p)...);
    }

    std::vector<Config> scopes(const std::string& path) const
    {
        return this->scopes(path.c_str());
    }

    std::vector<Config> scopes(const char* path) const
    {
        std::vector<Config> result;
        if(node)
        {
            const auto& nodes = node->getNodes(path, separator);
            for (const auto& sub : nodes)
            {
                result.push_back(Config(sub, separator));
            }
        }
        return result;
    }

    template<typename S>
    friend S& operator<<(S& stream, const Config& cfg)
    {
        stream << *cfg.node;
        return stream;
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(node);
    }

private:
    friend class ConfigBuilder;

    Config(const detail::ConfigNode::node_type& node, const  Separator& separator)
        : separator(separator)
        , node(node)
    {
    }

    template<typename T>
    std::string conversionError(const std::string& value, std::string&& path) const
    {
        return std::string("cannot convert \"") + value + "\" (path=" + path + ")"
               " to type " + boost::typeindex::type_id<T>().pretty_name();
    }

    Separator separator;
    detail::ConfigNode::node_type node;
};

template<>
inline boost::optional<std::string> Config::get<std::string>(const char* path) const
{
    if(node)
    {
        const auto& values = node->getValues(path, separator);
        if (!values.empty())
        {
            return boost::optional<std::string>(values[0]);
        }
    }
    return {};
}

template<>
inline boost::optional<bool> Config::get<bool>(const char* path) const
{
    if (auto&& value = this->get<std::string>(path))
    {
        if (*value == "true")
        {
            return true;
        }
        if (*value == "false")
        {
            return false;
        }

        return boost::lexical_cast<bool>(*value);
    }

    return {};
}

template<>
inline std::vector<std::string> Config::getAll<std::string>(const char* path) const
{
    static std::vector<std::string> empty;
    return (node)
        ? node->getValues(path, separator)
        : empty;
}

} //namespace dconfig

