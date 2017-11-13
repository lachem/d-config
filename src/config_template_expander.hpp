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
                result->emplace_back(Replacement{std::string((++what.begin())->str().c_str()), &parent});
            }
        }

        void visit(detail::ConfigNode&, const std::string&, detail::ConfigNode& node)
        {
            node.accept(*this);
        }

        detail::ConfigNode* root;
        const boost::xpressive::sregex* match;
        std::vector<Replacement>* result;
    };

public:
    explicit ConfigTemplateExpander(const Separator& aSeparator)
        : separator(aSeparator)
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
            if (!with.empty())
            {
                replacement.at->swap(*with[0]->clone());
            }
        }
    }

private:
    Separator separator;
};

} //namespace dconfig
