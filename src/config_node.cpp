//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <config_node.hpp>

namespace dconfig {
namespace detail {

ConfigNode::node_type ConfigNode::clone() const
{
    auto cloned = ConfigNode::node_type(new ConfigNode());
    for(auto& elem : nodes.get<ordered>())
    {
        node_list nodes;
        for (auto&& node : elem.value)
        {
            nodes.push_back(std::move(node->clone()));
        }
        cloned->nodes.get<sequenced>().push_back({elem.key, std::move(nodes)});
    }

    cloned->values = values;
    cloned->updateParents();

    return cloned;
}

void ConfigNode::overwrite(ConfigNode&& other)
{
    if (this->empty())
    {
        *this = std::move(other);
        return;
    }

    {
        auto currMergeTo = this->nodes.get<ordered>().begin();
        auto endMergeTo  = this->nodes.get<ordered>().end();

        auto currMergeFrom = other.nodes.get<ordered>().begin();
        auto endMergeFrom  = other.nodes.get<ordered>().end();

        while(currMergeTo != endMergeTo && currMergeFrom != endMergeFrom)
        {
            if(currMergeTo->key > currMergeFrom->key)
            {
                currMergeTo = this->nodes.insert(currMergeTo, {currMergeFrom->key, currMergeFrom->value});
                ++currMergeTo;
                ++currMergeFrom;
            }
            else
            if(currMergeTo->key == currMergeFrom->key)
            {
                //NOTE: const_cast is safe here as we are not touching the key
                auto update = const_cast<node_list&>(currMergeTo->value);
                for (size_t i = 0; i < std::min(update.size(), currMergeFrom->value.size()); ++i)
                {
                    update[i]->overwrite(std::move(*(currMergeFrom->value[i])));
                }
                for (size_t i = update.size(); i < currMergeFrom->value.size(); ++i)
                {
                    update.push_back(std::move(currMergeFrom->value[i]));
                }
                ++currMergeFrom;
                ++currMergeTo;
            }
            else
            if(currMergeTo->key < currMergeFrom->key)
            {
                ++currMergeTo;
            }
        }

        for(;currMergeFrom != endMergeFrom; ++currMergeFrom)
        {
            this->nodes.insert({currMergeFrom->key, currMergeFrom->value});
        }
    }

    {
        auto currMergeTo = this->values.get<ordered>().begin();
        auto endMergeTo  = this->values.get<ordered>().end();

        auto currMergeFrom = other.values.get<ordered>().begin();
        auto endMergeFrom  = other.values.get<ordered>().end();

        while(currMergeTo != endMergeTo && currMergeFrom != endMergeFrom)
        {
            if(currMergeTo->key > currMergeFrom->key)
            {
                currMergeTo = this->values.insert(currMergeTo, {currMergeFrom->key, currMergeFrom->value});
                ++currMergeTo;
                ++currMergeFrom;
            }
            else
            if(currMergeTo->key == currMergeFrom->key)
            {
                const_cast<value_list&>(currMergeTo->value) = std::move(const_cast<value_list&>(currMergeFrom->value));
                ++currMergeFrom;
                ++currMergeTo;
            }
            else
            if(currMergeTo->key < currMergeFrom->key)
            {
                ++currMergeTo;
            }
        }

        for(;currMergeFrom != endMergeFrom; ++currMergeFrom)
        {
            this->values.insert({currMergeFrom->key, currMergeFrom->value});
        }
    }

    this->updateParents();
}

} //namespace detail
} //namespace dconfig

