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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

//std
#include <cassert>
#include <queue>
#include <string>
#include <vector>
#include <memory>
#include <utility>

namespace dconfig {
namespace detail {

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

    return result;
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

boost::property_tree::ptree buildPropertyTree(const std::string& contents)
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
void ConfigRoot::parse(const std::vector<std::string>& contents)
{
    for(auto&& config : contents)
    {
        auto expanded = config;
        EnvVarExpander()(expanded);

        boost::trim(expanded);
        auto mergeFrom = buildCustomTree(buildPropertyTree(expanded));
        node.overwrite(std::move(*mergeFrom));
    }

    if(!node.empty())
    {
        ConfigParamExpander()(node);
        ConfigNodeExpander()(node);
    }
}

} //namespace detail
} //namespace dconfig
