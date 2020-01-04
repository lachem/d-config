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
        std::string with;
        detail::ConfigNode* at;
    };

    struct Visitor
    {
        explicit Visitor(detail::ConfigNode* root, const boost::xpressive::sregex* match, std::vector<Replacement>* result)
            : root(root)
            , match(match)
            , result(result)
        {
            assert(root);
            assert(match);
            assert(result);
        }

        void visit(detail::ConfigNode& parent, const std::string& key, size_t, std::string&)
        {
            using namespace boost::xpressive;

            smatch what;
            if (regex_match(key, what, *match))
            {
                assert(what.size()>1);
                result->emplace_back(Replacement{key, std::string(what[1].str().c_str()), &parent});
            }
        }

        void visit(detail::ConfigNode& parent, const std::string& key, size_t, detail::ConfigNode& node)
        {
            using namespace boost::xpressive;

            smatch what;
            if (regex_match(key, what, *match))
            {
                assert(what.size()>1);
                result->emplace_back(Replacement{key, std::string(what[1].str().c_str()), &parent});
            }
            else // TODO: Add support for nested templates?
            {
                node.accept(*this);
            }
        }

        detail::ConfigNode* root;
        const boost::xpressive::sregex* match;
        std::vector<Replacement>* result;
    };

public:

    explicit ConfigTemplateExpander(const Separator& separator, const char* prefix = "template")
        : separator(separator)
        , prefix(std::string("%") + prefix)
    {
        this->prefix += separator.value;
    }

    void operator()(detail::ConfigNode& root) const
    {
        using namespace boost::xpressive;

        sregex match =
                *(blank | _ln) >>
                (prefix.c_str()) >>
                (s1 = -+_) >>
                "%" >> *(blank | _ln);

        std::vector<Replacement> replacements;
        root.accept(Visitor(&root, &match, &replacements));
        for (auto& replacement : replacements)
        {
            const auto& with = root.getNodes(replacement.with, separator);
            if (with.size() >= 1)
            {
                auto&& clone = with[0]->clone();
                auto&& overwrite = replacement.at->getNodes(boost::string_ref(replacement.key));
                if (!overwrite.empty())
                {
                    clone->overwrite(std::move(*overwrite[0]));
                }
                replacement.at->swap(std::move(*clone));
            }
            else
            {
                throw std::invalid_argument(
                    std::string("Could not resolve \"") + replacement.key + "\"");
            }
        }
    }

private:
    Separator separator;
    std::string prefix;
};

} //namespace dconfig
