//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include "config.hpp"
#include "envvar_expander.hpp"

//std
#include <cassert>

//boost
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/xpressive/regex_primitives.hpp>

namespace dconfig
{
namespace detail
{

// --------------------------------------------------------------------------------
void ConfigRoot::parse(const std::vector<std::string>& contents)
{
    for(auto&& config : contents)
    {
        auto&& expanded = EnvVarExpander()(config);
        boost::trim(expanded);
        auto mergeFrom = buildPropertyTree(expanded);
        mergePropertyTree(ptree,mergeFrom);
    }
    if(!contents.empty())
    {
        expandConfigParameters(ptree);
        expandConfigNodes(ptree);
    }
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

// --------------------------------------------------------------------------------
struct ConfigVarExpander
{
    ConfigVarExpander(const boost::property_tree::ptree* ptree) : ptree(ptree) {};

    std::string operator() (const boost::xpressive::smatch& what) const
    {
        assert(what.size()>1);

        try
        {
            return ptree->get<std::string>((++what.begin())->str().c_str());
        }
        catch(...)
        {
            return what.str();
        }
    }

    const boost::property_tree::ptree* ptree;
};


void ConfigRoot::expandConfigParameters(boost::property_tree::ptree& ptree)
{
    using namespace boost::xpressive;

    sregex configParamMatch = "%config." >> (s1 = -+_) >> "%";
    expandConfigParameters(ptree, ptree, configParamMatch);
}

void ConfigRoot::expandConfigParameters(
    const boost::property_tree::ptree& rootNode
  , boost::property_tree::ptree& currNode
  , const boost::xpressive::sregex& match)
{
    using namespace boost::xpressive;

    for(auto&& node : currNode)
    {
        if(node.second.empty())
        {
            std::string value = node.second.get_value<std::string>();
            node.second.put_value(regex_replace(value, match, ConfigVarExpander(&rootNode)));
        }
        else
        {
            expandConfigParameters(rootNode, node.second, match);
        }
    }
}

// --------------------------------------------------------------------------------
void ConfigRoot::expandConfigNodes(boost::property_tree::ptree& ptree)
{
    using namespace boost::xpressive;
    using namespace regex_constants;

    sregex configNodeMatch = *(blank | _ln) >> "%node." >> (s1 = -+_) >> "%" >> *(blank | _ln);
    expandConfigNodes(ptree, ptree, configNodeMatch);
}

void ConfigRoot::expandConfigNodes(
    const boost::property_tree::ptree& rootNode
  , boost::property_tree::ptree& currNode
  , const boost::xpressive::sregex& match)
{
    using namespace boost::xpressive;

    for(auto&& node : currNode)
    {
        if(node.second.empty())
        {
            auto&& value = node.second.get_value<std::string>();
            smatch what;
            if (regex_match(value, what, match))
            {
                assert(what.size()>1);

                auto&& position = (++what.begin())->str().c_str();
                auto&& subtree = rootNode.get_child_optional(position);
                if (subtree)
                {
                    currNode.put_child(node.first, *subtree);
                }
            }
        }
        else
        {
            expandConfigNodes(rootNode, node.second, match);
        }
    }
}

} //namespace detail
} //namespace dconfig

