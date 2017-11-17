//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <separator.hpp>

//boost
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/variant.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/functional/hash.hpp>

//std
#include <memory>
#include <utility>
#include <vector>
#include <string>
#include <functional>
#include <cstring>

namespace dconfig {
namespace detail {

class ConfigNode
{
    struct sequenced {};
    struct ordered {};
    struct referenced {};

public:
    using value_type = std::string;
    using node_type  = std::shared_ptr<ConfigNode>;

    using value_list = std::vector<value_type>;
    using node_list  = std::vector<node_type>;

    //TODO: Consider - for optimization reasons we might want to extend the variant with single value
    using node_value_list = boost::variant<node_list, value_list>;

    struct ElementType
    {
        struct HashByKeyRef : std::unary_function<boost::string_ref, size_t>
        {
            std::size_t operator()(const boost::string_ref& value) const
            {
                return boost::hash_range(value.begin(), value.end());
            }
        };

        boost::string_ref keyRef() const { return boost::string_ref(key); }

        std::string key;
        node_value_list value;
    };

    using children_container = boost::multi_index::multi_index_container
        < ElementType
        , boost::multi_index::indexed_by
            < boost::multi_index::ordered_unique
                < boost::multi_index::tag<ordered>
                , boost::multi_index::member<ElementType, std::string, &ElementType::key>>
            , boost::multi_index::hashed_unique
                < boost::multi_index::tag<referenced>
                , boost::multi_index::const_mem_fun<ElementType, boost::string_ref, &ElementType::keyRef>
                , ElementType::HashByKeyRef>
            , boost::multi_index::sequenced
                < boost::multi_index::tag<sequenced>> >>;

    using iterator       = decltype(std::declval<children_container>().get<sequenced>().begin());
    using const_iterator = decltype(std::declval<children_container>().get<sequenced>().cbegin());

    bool empty() const
    {
        return children.empty();
    }

    const_iterator cbegin() const
    {
        return children.get<sequenced>().begin();
    }

    const_iterator cend() const
    {
        return children.get<sequenced>().end();
    }

    const_iterator begin() const
    {
        return children.get<sequenced>().begin();
    }

    const_iterator end() const
    {
        return children.get<sequenced>().end();
    }

    template<typename S>
    friend S& operator << (S&& stream, const ConfigNode& node)
    {
        node.print(stream, "");
        return stream;
    }

    template<typename T>
    void setValue(const std::string& key, T&& value)
    {
        auto&& it = children.get<ordered>().find(key);
        if (it == children.get<ordered>().end())
        {
            children.get<sequenced>().push_back({key, value_list{std::forward<T>(value)}});
        }
        else
        {
            children.get<ordered>().modify(it,
                [&value](ElementType& element)
                {
                    if (auto list = boost::get<value_list>(&element.value))
                    {
                        list->emplace_back(std::forward<T>(value));
                    }
                    else
                    if (auto list = boost::get<node_list>(&element.value))
                    {
                        element.value = value_list{std::forward<T>(value)};
                    }
                });
        }
    }

    template<typename T>
    void setNode(const std::string& key, T&& node)
    {
        auto&& it = children.get<ordered>().find(key);
        if (it == children.get<ordered>().end())
        {
            children.get<sequenced>().push_back({key, node_list{std::forward<T>(node)}});
        }
        else
        {
            children.get<ordered>().modify(it,
                [&node](ElementType& element)
                {
                    if (auto list = boost::get<node_list>(&element.value))
                    {
                        list->emplace_back(std::forward<T>(node));
                    }
                    else
                    if (auto list = boost::get<value_list>(&element.value))
                    {
                        element.value = node_list{std::forward<T>(node)};
                    }
                });
        }
    }

    //FIXME: getValues and getNodes are somewhat incosistent, sometimes they are recursive
    //       and sometimes they just look into immediate children. We need to somehow
    //       distinguish one from the other api-wise
    template<typename... K>
    const value_list& getValues(const boost::string_ref& key1, const boost::string_ref& key2, K&&... keys) const
    {
        static const value_list empty;

        auto&& nodes = getNodes(key1);
        return (!nodes.empty())
            ? nodes[0]->getValues(key2, keys...)
            : noValues();
    }

    const value_list& getValues(const boost::string_ref& key) const
    {
        auto&& child = children.get<referenced>().find(key);
        if (child != children.get<referenced>().end())
        {
            if (auto&& elem = boost::get<value_list>(&child->value))
            {
                return *elem;
            }
        }

        return noValues();
    }

    const value_list& getValues(const char* key, Separator separator) const
    {
        if (key)
        {
            clearJustSeparatorKey(key, separator);
            if (auto&& first = std::strchr(key, separator))
            {
                auto&& nodes = this->getNodes(boost::string_ref(key, first - key));
                if (!nodes.empty())
                {
                    return nodes[0]->getValues(first + 1, separator);
                }
            }
            else
            {
                return this->getValues(boost::string_ref(key, std::strlen(key)));
            }
        }

        return noValues();
    }

    const value_list& getValues(const std::string& key, Separator separator) const
    {
        return this->getValues(key.c_str(), separator);
    }

    template<typename... K>
    const node_list& getNodes(const boost::string_ref& key1, const boost::string_ref& key2, K&&... keys) const
    {
        static const node_list empty;

        auto&& nodes = getNodes(key1);
        return (!nodes.empty())
            ? nodes[0]->getNodes(key2, keys...)
            : noNodes();
    }

    const node_list& getNodes(const boost::string_ref& key) const
    {
        auto&& child = children.get<referenced>().find(key);
        if (child != children.get<referenced>().end())
        {
            if (auto&& elem = boost::get<node_list>(&child->value))
            {
                return *elem;
            }
        }

        return noNodes();
    }

    const node_list& getNodes(const char* key, Separator separator) const
    {
        if (key)
        {
            clearJustSeparatorKey(key, separator);
            if (auto&& first = std::strchr(key, separator))
            {
                auto&& nodes = this->getNodes(boost::string_ref(key, first - key));
                if (!nodes.empty())
                {
                    return nodes[0]->getNodes(first + 1, separator);
                }
            }
            else
            {
                return this->getNodes(boost::string_ref(key, std::strlen(key)));
            }
        }

        return noNodes();
    }

    const node_list& getNodes(const std::string& key, Separator separator) const
    {
        return getNodes(key.c_str(), separator);
    }

    void erase(const std::string& key)
    {
        children.get<referenced>().erase(key);
    }

    void swap(ConfigNode&& other)
    {
        children.swap(other.children);
    }

    void overwrite(ConfigNode&& other);

    ConfigNode::node_type clone() const;

    template<typename V>
    void accept(V&& visitor)
    {
        for(auto&& child : *this)
        {
            if (auto&& elem = boost::get<value_list>(&child.value))
            {
                for (auto&& value : *elem)
                {
                    //NOTE: const_cast is safe here as we are not touching the key
                    visitor.visit(*this, child.key, const_cast<std::string&>(value));
                }
            }
            else
            if (auto&& elem = boost::get<node_list>(&child.value))
            {
                for (auto&& node : *elem)
                {
                    visitor.visit(*this, child.key, *node);
                }
            }
        }
    }

private:
    template<typename S>
    void print(S& stream, const std::string& indent) const
    {
        for (const auto& child : children.get<sequenced>())
        {
            if (auto&& elem = boost::get<value_list>(&child.value))
            {
                stream << std::endl << indent << child.key << " = [";
                for (const auto& value : *elem)
                {
                    stream << value << ",";
                }
                stream << "]";
            }
            else
            if (auto&& elem = boost::get<node_list>(&child.value))
            {
                stream << std::endl << indent << child.key;
                for (const auto& node : *elem)
                {
                    node->print(stream, indent + "    ");
                }
            }
        }
    }

    static void clearJustSeparatorKey(const char*& key, Separator separator)
    {
        key = (key[0] != '\0' && key[1] == '\0' && key[0] == separator) ? "" : key;
    }

    static const value_list& noValues()
    {
        static const value_list empty;
        return empty;
    }

    static const node_list& noNodes()
    {
        static const node_list empty;
        return empty;
    }

    children_container children;
};

} //namespace detail
} //namespace dconfig

