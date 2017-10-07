//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <node.hpp>

namespace dconfig {
namespace detail {

void Node::overwrite(Node&& other) 
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
        if(currMergeTo->key > currMergeFrom->key)
        {
            currMergeTo = this->children.insert(currMergeTo, {currMergeFrom->key, currMergeFrom->value});
            ++currMergeTo;
            ++currMergeFrom;                
        }
        else
        if(currMergeTo->key == currMergeFrom->key)
        {
            auto&& nodeListTo   = boost::get<node_list>(&currMergeTo->value);
            auto&& nodeListFrom = boost::get<node_list>(&currMergeFrom->value);
            if (nodeListTo && nodeListFrom)
            {
                //NOTE: const_cast is safe here as we are not touching the key
                auto update = const_cast<node_list*>(nodeListTo);
                for (size_t i = 0; i < std::min(update->size(), nodeListFrom->size()); ++i)
                {
                    (*update)[i]->overwrite(std::move(*(*nodeListFrom)[i]));
                }
                for (size_t i = update->size(); i < nodeListFrom->size(); ++i)
                {
                    update->emplace_back(std::move((*nodeListFrom)[i]));
                }
            }
            else
            {   
                //NOTE: const_cast is safe here as we are not touching the key
                const_cast<node_value_list&>(currMergeTo->value) = 
                    std::move(const_cast<node_value_list&>(currMergeFrom->value));                    
            }
            ++currMergeFrom;
            ++currMergeTo;
        }
        else
        if(currMergeTo->key < currMergeFrom->key)
        {
            ++currMergeTo;
        }
    }
    
    for(;currMergeFrom != endMergeFrom; ++currMergeFrom)
    {
        this->children.insert({currMergeFrom->key, currMergeFrom->value});            
    }
}

} //namespace detail
} //namespace dconfig

