//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <config_node.hpp>
#include <separator.hpp>

//boost
#include <boost/property_tree/ptree.hpp>
#include <boost/xpressive/xpressive.hpp>

//std
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace dconfig {

class ConfigParamExpander
{
    struct RegexExpander
    {
        RegexExpander(
                const detail::ConfigNode* root,
                const detail::ConfigNode* node,
                const Separator& separator,
                const std::string* levelUp)
            : root(root)
            , node(node)
            , separator(separator)
            , levelUp(levelUp)
        {
            assert(root);
            assert(node);
            assert(levelUp);
        }

        const std::string& operator() (const boost::xpressive::smatch& what) const
        {
            assert(what.size()>3);

            auto scope = resolveScope(what);
            if (scope && what.size() >= 4)
            {
                auto&& path = what[3].str();
                auto&& values = scope->getValues(path.c_str(), separator);
                if (!values.empty())
                {
                    return values[0];
                }

                if (scope == root)
                {
                    auto&& values = node->getValues(path.c_str(), separator);
                    if (!values.empty())
                    {
                        return values[0];
                    }
                }
            }

            throw std::invalid_argument("");
        }

    private:
        const detail::ConfigNode* resolveScope(const boost::xpressive::smatch& what) const
        {
            if (what.size() < 3)
                return nullptr;

            auto&& currentLevel = what[1].str();
            auto&& parentLevel = what[2].str();

            auto scope = currentLevel.empty() && parentLevel.empty() ? root : node;
            if (!parentLevel.empty())
            {
                auto count = (parentLevel.size() / levelUp->size());
                for (size_t i = 0; i < count && scope; ++i)
                {
                    scope = scope->getParent() ? scope->getParent().get() : nullptr;
                }
            }

            return scope;
        }

        const detail::ConfigNode* root;
        const detail::ConfigNode* node;
        Separator separator;
        const std::string* levelUp;
    };

    struct Visitor
    {
        Visitor(
                detail::ConfigNode* root,
                const boost::xpressive::sregex* match,
                const Separator& separator,
                const std::string* levelUp)
            : match(match)
            , root(root)
            , separator(separator)
            , levelUp(levelUp)
        {
            assert(root);
            assert(match);
            assert(levelUp);
        }

        void visit(detail::ConfigNode::node_type const& parent, const std::string& key, size_t index, std::string& value)
        {
            try
            {
                value = boost::xpressive::regex_replace(value, *match, RegexExpander(root, parent.get(), separator, levelUp));
            }
            catch (const std::invalid_argument&)
            {
                throw std::invalid_argument(
                        std::string("Could not find \"") +
                        value + "\" to inject at \"" +
                        key + "[" + std::to_string(index) + "]\"");
            }
        }

        void visit(detail::ConfigNode::node_type const&, const std::string&, size_t, detail::ConfigNode::node_type const& node)
        {
            node->accept(*this);
        }

        const boost::xpressive::sregex* match;
        detail::ConfigNode* root;
        Separator separator;
        const std::string* levelUp;
    };

public:
    explicit ConfigParamExpander(const Separator& separator, const char* prefix = "config", const char level = '\0')
        : separator(separator)
        , prefix(std::string("%") + prefix)
    {
        if (level == '\0')
        {
            this->prefix += separator.value;
            this->levelUp += separator.value;
        }
        else
        {
            this->levelUp += level + level + separator.value;
            this->currentLevel += level + separator.value;
        }
    }

    void operator()(detail::ConfigNode& root) const
    {
        using namespace boost::xpressive;

        if (!currentLevel.empty())
        {
            sregex match =
                (prefix.c_str()) >>
                (s1 = *as_xpr(currentLevel.c_str())) >>
                (s2 = *as_xpr(levelUp.c_str())) >>
                (s3 = -+_) >> "%";
            root.accept(Visitor(&root, &match, separator, &levelUp));
        }
        else
        {
            sregex match =
                (prefix.c_str()) >>
                (s2 = *as_xpr(levelUp.c_str())) >>
                (s3 = -+_) >> "%";
            root.accept(Visitor(&root, &match, separator, &levelUp));
        }
    }

private:
    Separator separator;
    std::string prefix;
    std::string levelUp;
    std::string currentLevel;
};

} //namespace dconfig

