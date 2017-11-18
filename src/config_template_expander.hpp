//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include <separator.hpp>

//boost
#include <boost/property_tree/ptree.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_primitives.hpp>

//std
#include <cassert>

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

        void visit(detail::ConfigNode& parent, const std::string& key, std::string&)
        {
            using namespace boost::xpressive;

            smatch what;
            if (regex_match(key, what, *match))
            {
                assert(what.size()>1);
                result->emplace_back(Replacement{key, std::string(what[1].str().c_str()), &parent});
            }
        }

        void visit(detail::ConfigNode& parent, const std::string& key, detail::ConfigNode& node)
        {
            using namespace boost::xpressive;

            smatch what;
            if (regex_match(key, what, *match))
            {
                assert(what.size()>1);
                result->emplace_back(Replacement{key, std::string(what[1].str().c_str()), &parent});
            }
            else // TODO: Should I add support for nested templates?
            {
                node.accept(*this);
            }
        }

        detail::ConfigNode* root;
        const boost::xpressive::sregex* match;
        std::vector<Replacement>* result;
    };

public:
    explicit ConfigTemplateExpander(const Separator& separator)
        : separator(separator)
    {
    }

    void operator()(detail::ConfigNode& root)
    {
        using namespace boost::xpressive;

        sregex match = *(blank | _ln) >> "%template." >> (s1 = -+_) >> "%" >> *(blank | _ln);

        std::vector<Replacement> replacements;
        root.accept(Visitor(&root, &match, &replacements));
        for (auto& replacement : replacements)
        {
            const auto& with = root.getNodes(replacement.with, separator);
            if (with.size() == 1)
            {
                auto&& clone = with[0]->clone();
                auto&& overwrite = replacement.at->getNodes(boost::string_ref(replacement.key));
                if (!overwrite.empty())
                {
                    clone->overwrite(std::move(*overwrite[0]));
                }
                replacement.at->swap(std::move(*clone));
            }
        }
    }

private:
    Separator separator;
};

} //namespace dconfig
