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
#include <stdexcept>

namespace dconfig {

class ConfigParamExpander
{
    struct RegexExpander
    {
        RegexExpander(const detail::ConfigNode* root, const detail::ConfigNode* node, const Separator& separator, std::string* levelUp)
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

            auto scope = root;
            if (!what[1].str().empty())
            {
                scope = node;
            }

            if (!what[2].str().empty())
            {
                scope = node;
                auto count = (what[2].str().size() / levelUp->size());
                for (size_t i = 0; i < count && scope; ++i)
                {
                    scope = scope->getParent().get();
                }
            }

            assert(scope);
            auto&& values = scope->getValues(what[3].str().c_str(), separator);
            if (!values.empty())
            {
                return values[0];
            }

            //for backward compatiblity fallback to node scope
            assert(node);
            if (scope == root)
            {
                auto&& values = node->getValues(what[3].str().c_str(), separator);
                if (!values.empty())
                {
                    return values[0];
                }
            }

            throw std::invalid_argument("");

            static const std::string empty;
            return empty;
        }

        const detail::ConfigNode* root;
        const detail::ConfigNode* node;
        Separator separator;
        std::string* levelUp;
    };

    struct Visitor
    {
        Visitor(detail::ConfigNode* root, const boost::xpressive::sregex* match, const Separator& separator, std::string* levelUp)
            : match(match)
            , root(root)
            , separator(separator)
            , levelUp(levelUp)
        {
            assert(root);
            assert(match);
            assert(levelUp);
        }

        void visit(detail::ConfigNode& parent, const std::string& key, size_t index, std::string& value)
        {
            using namespace boost::xpressive;

            try
            {
                value = regex_replace(value, *match, RegexExpander(root, &parent, separator, levelUp));
            }
            catch (const std::invalid_argument&)
            {
                throw std::invalid_argument(
                        std::string("Could not find \"") + value + "\" to inject at \"" +
                        key + "[" + std::to_string(index) + "]\"");
            }
        }

        void visit(detail::ConfigNode&, const std::string&, size_t, detail::ConfigNode& node)
        {
            node.accept(*this);
        }

        const boost::xpressive::sregex* match;
        detail::ConfigNode* root;
        Separator separator;
        std::string* levelUp;
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

    void operator()(detail::ConfigNode& root)
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

