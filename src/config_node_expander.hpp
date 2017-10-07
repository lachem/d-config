//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//boost
#include <boost/property_tree/ptree.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_primitives.hpp>

//std
#include <cassert>

namespace dconfig {


class ConfigNodeExpander
{
    struct Replacement
    {
        std::string   of;
        std::string   with;
        detail::Node* at;
    };

    struct Visitor
    {
        explicit Visitor(detail::Node* root, const boost::xpressive::sregex* match, std::vector<Replacement>* result)
            : root(root)
            , match(match)
            , result(result)
        {
            assert(root);
            assert(match);
            assert(result);
        }

        void visit(detail::Node& parent, const std::string& key, std::string& value)
        {
            using namespace boost::xpressive;

            smatch what;
            if (regex_match(value, what, *match))
            {
                assert(what.size()>1);
                result->emplace_back(Replacement{key, std::string((++what.begin())->str().c_str()), &parent});
            }
        }

        void visit(detail::Node&, const std::string&, detail::Node& node)
        {
            node.accept(*this);
        }

        detail::Node* root;
        const boost::xpressive::sregex* match;
        std::vector<Replacement>* result;
    };

public:
    void operator()(detail::Node& root)
    {
        using namespace boost::xpressive;

        sregex match = *(blank | _ln) >> "%node." >> (s1 = -+_) >> "%" >> *(blank | _ln);

        root.accept(Visitor(&root, &match, &replacements));

        for (auto& replacement : replacements)
        {
            // TODO: make separator a parameter
            const auto& with = root.getNodes(replacement.with, '.');
            if (!with.empty())
            {
                replacement.at->setNode(replacement.of, with[0]);
            }
        }
    }

private:
    std::vector<Replacement> replacements;
};

} //namespace dconfig
