//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include "node.hpp"
#include "config.hpp"
#include "env_var_expander.hpp"
#include "config_param_expander.hpp"
#include "config_node_expander.hpp"

//boost
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

//std
#include <cassert>
#include <queue>
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <utility>

namespace dconfig {
namespace detail {

template<typename S>
void print(S&& stream, const boost::property_tree::ptree& source, const std::string& indent)
{
    for (const auto& node : source)
    {
        if (node.second.empty())
        {
            std::cout << std::endl << indent << node.first << " = " << node.second.get_value<std::string>();
        }
        else
        {
            std::cout << std::endl << indent << node.first;
            print(stream, node.second, indent + "    ");
        }
    }
}

template<typename S>
S& operator << (S&& stream, const boost::property_tree::ptree& source)
{
    print(stream, source, "");
    return stream;
}

// --------------------------------------------------------------------------------
std::shared_ptr<Node> buildCustomTree(const boost::property_tree::ptree& source)
{
    using element_type = std::pair<const boost::property_tree::ptree*, std::shared_ptr<Node>>;

    std::queue<element_type> queue;
    std::queue<element_type> next;

    auto result = std::make_shared<Node>();
    queue.push(std::make_pair(&source, result));
    while (!queue.empty())
    {
        for(auto&& node : *queue.front().first)
        {
            auto&& key = node.first;
            if (!node.second.empty())
            {
                auto&& insert = std::make_shared<Node>();
                queue.front().second->setNode(key, insert);
                next.push(std::make_pair(&node.second, insert));
            }
            else
            {
                auto&& value = node.second.get_value<std::string>();
                queue.front().second->setValue(key, std::move(value));
            }
        }

        queue.pop();
        if (queue.empty())
        {
            queue.swap(next);
        }
    }
        
    // std::cout << "*** PTREE ***"  << std::endl << source << std::endl;
    // std::cout << "*** CUSTOM ***" << std::endl << *result << std::endl;

    return result;
}

// --------------------------------------------------------------------------------
void ConfigRoot::parse(const std::vector<std::string>& contents)
{
    Node stopa;
    for(auto&& config : contents)
    {
        auto expanded = config;
        EnvVarExpander()(expanded);

        boost::trim(expanded);
        auto mergeFrom = buildPropertyTree(expanded);
        auto dupa = buildCustomTree(mergeFrom);
        
        mergePropertyTree(ptree,mergeFrom);        
        stopa.overwrite(std::move(*dupa));
    }
    
    std::cout << std::endl << "*** MERGED PTREE ***"  << ptree << std::endl;
    
    if(!contents.empty())
    {
        ConfigParamExpander()(ptree);
        ConfigNodeExpander()(ptree);
    }
    
    std::cout << std::endl << "*** MERGED CUSTOM ***" << stopa << std::endl;
}

// --------------------------------------------------------------------------------
inline bool isXml(const std::string& contents)
{
    return contents[0] == '<';
}

inline bool isJson(const std::string& contents)
{
    return contents[0] == '{';
}

boost::property_tree::ptree ConfigRoot::buildPropertyTree(const std::string& contents)
{
    std::stringstream contentStream;
    contentStream.str(contents);
    boost::property_tree::ptree ptree;
    if (isXml(contents))
    {
        boost::property_tree::xml_parser::read_xml(contentStream, ptree);
    }
    else
    if (isJson(contents))
    {
        boost::property_tree::json_parser::read_json(contentStream, ptree);
    }
    else
    {
        // TODO: inform the user
    }

    return ptree;
}

// --------------------------------------------------------------------------------
void ConfigRoot::mergePropertyTree(
    boost::property_tree::ptree& mergeTo
  , const boost::property_tree::ptree& mergeFrom)
{
    using namespace boost::property_tree;

    auto currMergeTo = mergeTo.ordered_begin();
    auto endMergeTo = mergeTo.not_found();

    auto currMergeFrom = mergeFrom.ordered_begin();
    auto endMergeFrom = mergeFrom.not_found();

    if(currMergeTo == endMergeTo)
    {
        mergeTo = mergeFrom;
    }

    while(currMergeTo != endMergeTo && currMergeFrom != endMergeFrom)
    {
        auto itMergeTo   = mergeTo.to_iterator(currMergeTo);
        auto itMergeFrom = mergeFrom.to_iterator(currMergeFrom);

        if(itMergeTo->first > itMergeFrom->first)
        {
            mergeTo.add_child(itMergeFrom->first, itMergeFrom->second);
            ++currMergeFrom;
            continue;
        }

        if(itMergeTo->first == itMergeFrom->first)
        {
            if(itMergeFrom->second.empty())
            {
                mergeTo.put("", itMergeFrom->second.data());
            }
            mergePropertyTree(itMergeTo->second, itMergeFrom->second);
            ++currMergeFrom;
        }

        if(itMergeTo->first <= itMergeFrom->first)
        {
            ++currMergeTo;
        }
    }

    for(;currMergeFrom != endMergeFrom; ++currMergeFrom)
    {
        auto itMergeFrom = mergeFrom.to_iterator(currMergeFrom);
        mergeTo.add_child(itMergeFrom->first, itMergeFrom->second);
    }
}

} //namespace detail
} //namespace dconfig
