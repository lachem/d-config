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
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/xpressive/xpressive.hpp>

namespace config 
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
    
    ConfigRoot(const std::vector<std::string>& aFileList, Separator aSeparator = Separator()) 
        : files(aFileList)
        , separator(aSeparator) 
    {
        parse();
    }
    
    ConfigRoot(ConfigRoot&&) = delete;
    ConfigRoot(const ConfigRoot&) = delete;
    bool operator=(const ConfigRoot&) = delete;
    
    const tree_type& getTree() const { return ptree; }
    const std::vector<std::string>& used() const { return files; }
 
private: 
    void parse();
    std::string readConfigFile(const std::string& filename);
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
   
    boost::property_tree::ptree ptree;
    std::vector<std::string> files;
    Separator separator;
};

} //namespace detail

struct Config
{    
    typedef detail::ConfigRoot::Separator separator_type;
    typedef detail::ConfigRoot::tree_type tree_type;
    
    Config(const std::vector<std::string>& aFileList, const separator_type& aSeparator = separator_type())
        : root(new detail::ConfigRoot(aFileList,aSeparator))
        , tree(root->getTree())
        , separator(aSeparator)
    {}
    
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
        auto subtree = tree;
        auto split = splitPath(path);
        if(!split.first.empty())
        {
            subtree = getSubtree(split.first);
        }
        if(subtree)
        {
            auto range = subtree->equal_range(split.second);
            for (; range.first != range.second; ++range.first) 
            {
                auto it = subtree->to_iterator(range.first);
                result.push_back(it->second.get_value<T>());
            }
            
        }
        return std::move(result);        
    }
    
    Config scope(const std::string& path) const 
    { 
        return Config(root, getSubtree(path), separator);         
    }
    
    std::vector<Config> scopes(const std::string& path) const 
    { 
        std::vector<Config> result;
        auto subtree = tree;
        auto split = splitPath(path);
        if(!split.first.empty())
        {
            subtree = getSubtree(split.first);
        }
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
    
    const std::vector<std::string>& used() const 
    { 
        return root->used();        
    }
    
    explicit operator bool() const noexcept
    { 
        return tree;
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

Config init(int argc, char** argv);

} //namespace config
