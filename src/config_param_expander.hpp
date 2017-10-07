//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include <node.hpp>

//boost
#include <boost/property_tree/ptree.hpp>
#include <boost/xpressive/xpressive.hpp>

//std
#include <cassert>
#include <iostream>

namespace dconfig {

class ConfigParamExpander
{
    struct RegexExpander
    {
        explicit RegexExpander(const boost::property_tree::ptree* ptree)
            : ptree(ptree)
        {
        }

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

public:
    void operator()(boost::property_tree::ptree& ptree)
    {
        using namespace boost::xpressive;

        sregex configParamMatch = "%config." >> (s1 = -+_) >> "%";
        expandConfigParams(ptree, ptree, configParamMatch);
    }

private:
    void expandConfigParams(
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
                node.second.put_value(regex_replace(value, match, RegexExpander(&rootNode)));
            }
            else
            {
                expandConfigParams(rootNode, node.second, match);
            }
        }
    }
};

class ConfigParamExpanderNew
{
    struct RegexExpander
    {
        explicit RegexExpander(const detail::Node* node)
            : node(node)
        {
        }

        const std::string& operator() (const boost::xpressive::smatch& what) const
        {
            static const std::string empty;
            assert(what.size()>1);

            // TODO: make separator a parameter
            auto&& values = node->getValues((++what.begin())->str().c_str(), '.');
            if (!values.empty())
            {
                return values[0];
            }

            return empty;
        }

        const detail::Node* node;
    };

    struct Visitor
    {
        explicit Visitor(detail::Node* root, const boost::xpressive::sregex* match)
            : match(match)
            , root(root)
        {
        }

        void visit(detail::Node&, const std::string&, std::string& value)
        {
            using namespace boost::xpressive;
            value = regex_replace(value, *match, RegexExpander(root));
        }

        void visit(detail::Node&, const std::string&, detail::Node& node)
        {
            node.accept(*this);
        }

        const boost::xpressive::sregex* match;
        detail::Node* root;
    };

public:
    void operator()(detail::Node& root)
    {
        using namespace boost::xpressive;

        sregex match = "%config." >> (s1 = -+_) >> "%";

        root.accept(Visitor(&root, &match));
    }
};

} //namespace dconfig

