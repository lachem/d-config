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

class ConfigLocalExpander
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
            assert(what.size()>2);

            auto scope = node;
            for (size_t i=0; i<what[1].str().size() && scope; ++i)
            {
                scope = scope->getParent().get();
            }

            assert(scope);
            auto&& values = scope->getValues(what[2].str().c_str(), separator);
            if (!values.empty())
            {
                return values[0];
            }

            static const std::string empty;
            return empty;
        }

        const detail::ConfigNode* node;
        Separator separator;
    };

    struct Visitor
    {
        Visitor(const boost::xpressive::sregex* match, const Separator& separator)
            : match(match)
            , separator(separator)
        {
        }

        void visit(detail::ConfigNode& parent, const std::string&, std::string& value)
        {
            using namespace boost::xpressive;
            value = regex_replace(value, *match, RegexExpander(&parent, separator));
        }

        void visit(detail::ConfigNode&, const std::string&, detail::ConfigNode& node)
        {
            node.accept(*this);
        }

        const boost::xpressive::sregex* match;
        Separator separator;
    };

public:
    explicit ConfigLocalExpander(const Separator& separator)
        : separator(separator)
    {
    }

    void operator()(detail::ConfigNode& root)
    {
        using namespace boost::xpressive;

        sregex match = "%local." >> (s1 = *as_xpr(static_cast<char>(separator))) >> (s2 = -+_) >> "%";
        root.accept(Visitor(&match, separator));
    }

private:
    Separator separator;
};

} //namespace dconfig

