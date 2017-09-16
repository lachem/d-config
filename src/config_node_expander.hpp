//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//boost
#include <boost/property_tree/ptree.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_primitives.hpp>

//std
#include <cassert>

namespace dconfig {

class ConfigNodeExpander
{
public:
    void operator()(boost::property_tree::ptree& ptree)
    {
        using namespace boost::xpressive;
        using namespace regex_constants;

        sregex configNodeMatch = *(blank | _ln) >> "%node." >> (s1 = -+_) >> "%" >> *(blank | _ln);
        expandConfigNodes(ptree, ptree, configNodeMatch);
    }

private:
    void expandConfigNodes(
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
};

} //namespace dconfig

