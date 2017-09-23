#pragma once

//boost
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp> 
#include <boost/multi_index/member.hpp> 
#include <boost/variant.hpp>

//std
#include <memory>
#include <utility>
#include <string>
#include <vector>

namespace dconfig {
namespace detail {

class Node
{
    struct sequenced {};
    struct ordered {};

    using value_type = std::string;
    using node_type  = std::shared_ptr<Node>;
    
    using value_list = std::vector<value_type>;
    using node_list  = std::vector<node_type>;
    
    //TODO: For optimization reasons we might want to extend the variant with single value
    using node_value_list = boost::variant<node_list, value_list>;
    using element_type = std::pair<std::string, node_value_list>;
    
    using children_container = boost::multi_index::multi_index_container
        < element_type
        , boost::multi_index::indexed_by
            < boost::multi_index::ordered_unique
                < boost::multi_index::tag<ordered>                
                , boost::multi_index::member<element_type, std::string, &element_type::first>>
            , boost::multi_index::sequenced 
                < boost::multi_index::tag<sequenced>> >>;
                
public:
    using iterator       = decltype(std::declval<children_container>().get<sequenced>().begin());
    using const_iterator = decltype(std::declval<children_container>().get<sequenced>().cbegin());
    
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
                [&value](element_type& element)
                {
                    if (auto list = boost::get<value_list>(&element.second))
                    {
                        list->emplace_back(std::forward<T>(value));
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
                [&node](element_type& element)
                {
                    if (auto list = boost::get<node_list>(&element.second))
                    {
                        list->emplace_back(std::forward<T>(node));
                    }
                });
        }
    }
    
    void overwrite(Node&& other) 
    {
        auto currMergeTo = this->children.get<ordered>().begin();
        auto endMergeTo  = this->children.get<ordered>().end();
        
        auto currMergeFrom = other.children.get<ordered>().begin();
        auto endMergeFrom  = other.children.get<ordered>().end();

        if(currMergeTo == endMergeTo)
        {
            *this = std::move(other);
        }

        while(currMergeTo != endMergeTo && currMergeFrom != endMergeFrom)
        {
            if(currMergeTo->first > currMergeFrom->first)
            {
                currMergeTo = this->children.insert(currMergeTo, {currMergeFrom->first, currMergeFrom->second});
                ++currMergeTo;
                ++currMergeFrom;                
            }
            else
            if(currMergeTo->first == currMergeFrom->first)
            {
                auto&& nodeListTo   = boost::get<node_list>(&currMergeTo->second);
                auto&& nodeListFrom = boost::get<node_list>(&currMergeFrom->second);
                if (nodeListTo && nodeListFrom)
                {
                   (*nodeListTo)[0]->overwrite(std::move(*(*nodeListFrom)[0]));
                }
                else
                {                    
                    this->children.get<ordered>().modify(currMergeTo, 
                        [&currMergeFrom](element_type& element)
                        {
                                element.second = std::move(const_cast<node_value_list&>(currMergeFrom->second));
                        });
                }
                ++currMergeFrom;
                ++currMergeTo;
            }
            else
            if(currMergeTo->first < currMergeFrom->first)
            {
                ++currMergeTo;
            }
        }
        
        for(;currMergeFrom != endMergeFrom; ++currMergeFrom)
        {
            this->children.insert({currMergeFrom->first, currMergeFrom->second});            
        }
    }

    const const_iterator begin() const
    {
        return children.get<sequenced>().begin();
    }
    
    const const_iterator end() const
    {
        return children.get<sequenced>().end();
    }
    
    template<typename S>
    friend S& operator << (S&& stream, const Node& node)
    {
        node.print(stream, "");
        
        return stream;
    }
    
private:  
    template<typename S>
    void print(S& stream, const std::string& indent) const
    {
        for (const auto& child : children.get<sequenced>())
        {
            if (auto&& elem = boost::get<value_list>(&child.second))
            {
                stream << std::endl << indent << child.first << " = [";
                for (const auto& value : *elem)
                {
                    stream << value << ",";
                }
                stream << "]";
            }
            else
            if (auto&& elem = boost::get<node_list>(&child.second))
            {
                stream << std::endl << indent << child.first;
                for (const auto& node : *elem)
                {
                    node->print(stream, indent + "    ");
                }
            }
        }
    }

    children_container children;
};

} //namespace detail
} //namespace dconfig

