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
#include <unordered_map>
#include <vector>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace dconfig {

class ConfigNodeExpander
{
    struct KeyNode
    {
        std::string key;
        detail::ConfigNode::node_type node;
        size_t index;
    };

    using replacement_container = std::unordered_map<detail::ConfigNode*, std::vector<KeyNode>>;

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

        void visit(detail::ConfigNode::node_type const& parent, const std::string& key, size_t index, std::string& value)
        {
            using namespace boost::xpressive;

            smatch what;
            if (regex_match(value, what, *match))
            {
                if (what.size() > 3)
                {
                    auto&& path = what[3].str();
                    auto scope = resolveScope(what, parent.get());
                    if (scope && addKeyNode(scope, parent.get(), key, index, path))
                        return;

                    //for backward compatiblity fallback to node scope
                    if (scope == root && addKeyNode(parent.get(), parent.get(), key, index, path))
                        return;
                }

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

    private:
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

        bool addKeyNode(detail::ConfigNode* scope, detail::ConfigNode* parent, const std::string& key, size_t index, const std::string& from)
        {
            auto&& nodes = scope->getNodes(from.c_str(), separator);
            if (!nodes.empty())
            {
                (*result)[parent].push_back(KeyNode{key, nodes[0], index});
                return true;
            }

            if (nodes.empty())
            {
                auto&& values = scope->getValues(from.c_str(), separator);
                if (values.size() == 1 && values[0].empty())
                {
                    (*result)[parent].push_back(KeyNode{key, detail::ConfigNode::create(), index});
                    return true;
                }
            }

            if (auto&& baseKeyNode = getBaseNode(scope, from);
                baseKeyNode.node)
            {
                auto findIt = result->find(baseKeyNode.node.get());
                if (findIt != result->end())
                {
                    for (auto&& replacement : findIt->second)
                    {
                        if (replacement.key == baseKeyNode.key)
                        {
                            (*result)[parent].push_back(KeyNode{key, replacement.node, index});
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        //TODO: consider moving this to ConfigNode
        KeyNode getBaseNode(detail::ConfigNode* scope, const std::string& key) const
        {
            auto&& values = scope->getValues(key.c_str(), separator);
            if (!values.empty())
            {
                if (auto&& last = std::strrchr(key.c_str(), separator.value))
                {
                    auto&& nodes = scope->getNodes(boost::string_ref(key.c_str(), last - key.c_str()));
                    if (!nodes.empty())
                    {
                        return {last + 1, nodes[0], 0};
                    }
                }
            }
            return {};
        }

        detail::ConfigNode* root;
        const boost::xpressive::sregex* match;
        replacement_container* result;
        Separator separator;
        const std::string* levelUp;
    };

public:
    explicit ConfigNodeExpander(const Separator& separator, const char* prefix = "node", const char level = '\0')
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

        for (auto& replacementAt : replacements)
        {
            // sort the array in order to remove the indicies from the last one
            std::sort(replacementAt.second.begin(), replacementAt.second.end(),
                    [](const KeyNode& lhs, const KeyNode& rhs) { return lhs.index > rhs.index; });
            for (const auto& replacement : replacementAt.second)
            {
                if (replacement.node)
                {
                    replacementAt.first->eraseValue(replacement.key, replacement.index);
                }
            }
            for (const auto& replacement : replacementAt.second)
            {
                if (replacement.node)
                {
                    replacementAt.first->setNode(replacement.key, replacement.node);
                }
            }
        }
    }

private:
    Separator separator;
    std::string prefix;
    std::string levelUp;
    std::string currentLevel;
};

} //namespace dconfig
