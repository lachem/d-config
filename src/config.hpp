//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//std
#include <map>
#include <memory>

//boost
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/xpressive/xpressive.hpp>

namespace dconfig
{
namespace detail
{

struct ConfigRoot
{
    struct Separator
    {
        Separator() : value(default_value()) {}
        Separator(const std::string& value) : value(value) {}
        Separator(const Separator&) = default;

        static std::string default_value() { return "."; }

        std::string value;
    };

    typedef boost::property_tree::ptree tree_type;

    explicit ConfigRoot(const std::vector<std::string>& contents, Separator aSeparator = Separator())
        : separator(aSeparator)
    {
        parse(contents);
    }

    ConfigRoot(ConfigRoot&&) = delete;
    ConfigRoot(const ConfigRoot&) = delete;
    ConfigRoot& operator=(const ConfigRoot&) = delete;

    const tree_type& getTree() const { return ptree; }

private:
    void parse(const std::vector<std::string>& contents);
    std::string expandEnvParameters(const std::string& contents);
    boost::property_tree::ptree buildPropertyTree(const std::string& contents);

    void mergePropertyTree(
        boost::property_tree::ptree& mergeTo
      , const boost::property_tree::ptree& currNode);

    void expandConfigParameters(
        boost::property_tree::ptree& ptree);
    void expandConfigParameters(
        const boost::property_tree::ptree& rootNode
      , boost::property_tree::ptree& currNode
      , const boost::xpressive::sregex& match);

    void expandConfigNodes(
        boost::property_tree::ptree& ptree);
    void expandConfigNodes(
        const boost::property_tree::ptree& rootNode
      , boost::property_tree::ptree& currNode
      , const boost::xpressive::sregex& match);

    boost::property_tree::ptree ptree;
    Separator separator;
};

} //namespace detail

struct Config
{
    typedef detail::ConfigRoot::Separator separator_type;
    typedef detail::ConfigRoot::tree_type tree_type;

    explicit Config(const std::vector<std::string>& aFileList, const separator_type& aSeparator = separator_type())
        : root(new detail::ConfigRoot(aFileList,aSeparator))
        , tree(root->getTree())
        , separator(aSeparator)
    {
    }

    template<typename T>
    boost::optional<T> get(const std::string& path) const
    {
        if(tree)
        {
            return (separator.value != separator_type::default_value())
                ? tree->get_optional<T>(boost::replace_all_copy(path, separator.value, "."))
                : tree->get_optional<T>(path);

        }
        return boost::optional<T>();
    }

    template<typename T>
    std::vector<T> getAll(const std::string& path) const
    {
        std::vector<T> result;
        auto split = splitPath(path);
        auto subtree = (!split.first.empty())
            ? getSubtree(split.first)
            : tree;
        if(subtree)
        {
            auto range = subtree->equal_range(split.second);
            for (; range.first != range.second; ++range.first)
            {
                auto it = subtree->to_iterator(range.first);
                result.push_back(it->second.get_value<T>());
            }

        }
        return result;
    }

    Config scope(const std::string& path) const
    {
        return Config(root, getSubtree(path), separator);
    }

    std::vector<Config> scopes(const std::string& path) const
    {
        std::vector<Config> result;
        auto split = splitPath(path);
        auto subtree = (!split.first.empty())
            ? getSubtree(split.first)
            : tree;
        if(subtree)
        {
            auto range = subtree->equal_range(split.second);
            for (; range.first != range.second; ++range.first)
            {
                auto it = subtree->to_iterator(range.first);
                result.push_back(Config(root, it->second ,separator));
            }
        }
        return std::move(result);
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(tree);
    }

private:
    Config(std::shared_ptr<detail::ConfigRoot> aConfigRoot
         , const boost::optional<const tree_type&>& aSubtree
         , const separator_type& aSeparator)
        : root(aConfigRoot)
        , tree(aSubtree)
        , separator(aSeparator)
    {}

    boost::optional<const tree_type&> getSubtree(const std::string& path) const
    {
        if(tree)
        {
            return (separator.value != separator_type::default_value())
                ? tree->get_child_optional(boost::replace_all_copy(path, separator.value, "."))
                : tree->get_child_optional(path);
        }
        return boost::optional<const tree_type&>();
    }

    std::pair<std::string, std::string> splitPath(const std::string& path) const
    {
        std::string name;
        std::string prefix;
        auto pos = path.rfind(separator.value);
        if(pos != std::string::npos)
        {
            return std::make_pair(std::move(path.substr(0,pos)),std::move(path.substr(pos+1, path.size()-pos-1)));
        }
        return std::make_pair(prefix,path);
    }

    std::shared_ptr<detail::ConfigRoot> root;
    boost::optional<const tree_type&> tree;
    separator_type separator;
};

} //namespace dconfig

