//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//local
#include <separator.hpp>

//boost
#include <boost/property_tree/ptree.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_primitives.hpp>

//std
#include <cassert>
#include <stdexcept>

namespace dconfig {

class ConfigTemplateExpander
{
    struct Replacement
    {
        std::string key;
        detail::ConfigNode::node_type with;
    };

    using replacement_container = std::unordered_map<detail::ConfigNode*, Replacement>;

    struct Visitor
    {
        explicit Visitor(detail::ConfigNode* root,
                const boost::xpressive::sregex* match,
                replacement_container* result,
                Separator separator,
                const std::string* levelUp)
            : root(root)
            , match(match)
            , result(result)
            , separator(separator)
            , levelUp(levelUp)
        {
            assert(root);
            assert(match);
            assert(result);
        }

        void visit(detail::ConfigNode& parent, const std::string& key, size_t, std::string&)
        {
            visit(parent, key);
        }

        void visit(detail::ConfigNode& parent, const std::string& key, size_t, detail::ConfigNode& node)
        {
            if (!visit(parent, key)) // TODO: Add support for nested templates?
            {
                node.accept(*this);
            }
        }

    private:
        bool visit(detail::ConfigNode& parent, const std::string& key)
        {
            using namespace boost::xpressive;

            smatch what;
            if (regex_match(key, what, *match))
            {
                if (what.size() > 3)
                {
                    auto&& path = what[3].str();
                    auto scope = resolveScope(what, &parent);
                    if (scope && addKeyNode(scope, &parent, key, path))
                        return true;

                    //for backward compatiblity fallback to node scope
                    if (scope == root && addKeyNode(&parent, &parent, key, path))
                        return true;
                }

                throw std::invalid_argument(
                    std::string("Could not resolve \"") + key + "\"");
            }

            return false;
        }

        detail::ConfigNode* resolveScope(const boost::xpressive::smatch& what, detail::ConfigNode* node) const
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

        bool addKeyNode(detail::ConfigNode* scope, detail::ConfigNode* parent, const std::string& key, const std::string& from)
        {
            auto&& nodes = scope->getNodes(from.c_str(), separator);
            if (!nodes.empty())
            {
                (*result)[parent] = Replacement{key, nodes[0]};
                return true;
            }

            if (nodes.empty())
            {
                auto&& values = scope->getValues(from.c_str(), separator);
                if (values.size() == 1 && values[0].empty())
                {
                    (*result)[parent] = Replacement{key, detail::ConfigNode::create()};
                    return true;
                }
            }

            return false;
        }

        detail::ConfigNode* root;
        const boost::xpressive::sregex* match;
        replacement_container* result;
        Separator separator;
        const std::string* levelUp;
    };

public:

    explicit ConfigTemplateExpander(const Separator& separator, const char* prefix = "template", const char level = '\0')
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

        replacement_container replacements;
        if (!currentLevel.empty())
        {
            sregex match =
                *(blank | _ln) >>
                (prefix.c_str()) >>
                (s1 = *as_xpr(currentLevel.c_str())) >>
                (s2 = *as_xpr(levelUp.c_str())) >>
                (s3 = -+_) >>
                "%" >> *(blank | _ln);
            root.accept(Visitor(&root, &match, &replacements, separator, &levelUp));
        }
        else
        {
            sregex match =
                *(blank | _ln) >>
                (prefix.c_str()) >>
                (s2 = *as_xpr(levelUp.c_str())) >>
                (s3 = -+_) >>
                "%" >> *(blank | _ln);
            root.accept(Visitor(&root, &match, &replacements, separator, &levelUp));
        }

        for (auto& replacement : replacements)
        {
            auto&& clone = replacement.second.with->clone();
            auto&& overwrite = replacement.first->getNodes(boost::string_ref(replacement.second.key));
            if (!overwrite.empty())
            {
                clone->overwrite(std::move(*overwrite[0]));
            }
            replacement.first->swap(std::move(*clone));
        }
    }

private:
    Separator separator;
    std::string prefix;
    std::string levelUp;
    std::string currentLevel;
};

} //namespace dconfig
