//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include <config_node.hpp>
#include <separator.hpp>

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
        RegexExpander(const detail::ConfigNode* node, const Separator& separator)
            : node(node)
            , separator(separator)
        {
        }

        const std::string& operator() (const boost::xpressive::smatch& what) const
        {
            static const std::string empty;
            assert(what.size()>1);

            auto&& values = node->getValues((++what.begin())->str().c_str(), separator);
            if (!values.empty())
            {
                return values[0];
            }

            return empty;
        }

        const detail::ConfigNode* node;
        Separator separator;
    };

    struct Visitor
    {
        Visitor(detail::ConfigNode* aRoot, const boost::xpressive::sregex* match, const Separator& separator)
            : match(match)
            , root(aRoot)
            , separator(separator)
        {
        }

        void visit(detail::ConfigNode&, const std::string&, std::string& value)
        {
            using namespace boost::xpressive;
            value = regex_replace(value, *match, RegexExpander(root, separator));
        }

        void visit(detail::ConfigNode&, const std::string&, detail::ConfigNode& node)
        {
            node.accept(*this);
        }

        const boost::xpressive::sregex* match;
        detail::ConfigNode* root;
        Separator separator;
    };

public:
    explicit ConfigParamExpander(const Separator& separator, const char* prefix = "config")
        : separator(separator)
        , prefix(std::string("%") + prefix + '.')
    {
    }

    void operator()(detail::ConfigNode& root)
    {
        using namespace boost::xpressive;

        sregex match = prefix.c_str() >> (s1 = -+_) >> '%';
        root.accept(Visitor(&root, &match, separator));
    }

private:
    Separator separator;
    std::string prefix;
};

} //namespace dconfig

