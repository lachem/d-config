//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include <config.hpp>
#include <config_node.hpp>
#include <config_builder.hpp>

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

// --------------------------------------------------------------------------------
ConfigBuilder::node_type buildCustomTree(const boost::property_tree::ptree& source, const ArrayKey& arrayKey)
{
    using element_type = std::pair<const boost::property_tree::ptree*, std::shared_ptr<detail::ConfigNode>>;

    std::queue<element_type> queue;
    std::queue<element_type> next;

    auto result = std::make_shared<detail::ConfigNode>();
    queue.push(std::make_pair(&source, result));
    while (!queue.empty())
    {
        for (auto&& node : *queue.front().first)
        {
            if (auto&& key = node.first;
                !node.second.empty())
            {
                auto&& insert = std::make_shared<detail::ConfigNode>();
                if (!key.empty() && key[0] != arrayKey.value)
                {
                    queue.front().second->setNode(key, insert);
                }
                else
                {
                    queue.front().second->setNode("", insert);
                }
                next.push(std::make_pair(&node.second, insert));
            }
            else
            {
                auto&& value = node.second.get_value<std::string>();
                if (!key.empty() && key[0] != arrayKey.value)
                {
                    queue.front().second->setValue(key, std::move(value));
                }
                else
                {
                    queue.front().second->setValue("", std::move(value));
                }
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
ConfigBuilder::node_type ConfigBuilder::parse(std::vector<std::string>&& contents)
{
    ConfigBuilder::node_type root;
    for (auto& expanded : contents)
    {
        for (auto& expander : preexpand)
        {
            expander(expanded);
        }
        boost::trim(expanded);

        if (auto mergeFrom = buildCustomTree(buildPropertyTree(expanded), arrayKey);
            !root)
        {
            root = mergeFrom;
        }
        else
        {
            root->overwrite(std::move(*mergeFrom));
        }
    }

    if (root && !root->empty())
    {
        for (auto& expander : postexpand)
        {
            expander(*root);
        }
    }

    return root;
}

} //namespace dconfig

