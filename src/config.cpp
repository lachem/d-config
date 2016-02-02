//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include "config.hpp"

//boost
#include <boost/program_options.hpp>

namespace config 
{

namespace po = boost::program_options;

namespace detail
{
    
struct EnvVarExpander
{
    std::string operator() (const boost::xpressive::smatch& what) const
    {
        assert(what.size()>1);
        
        const char* env = getenv((++what.begin())->str().c_str());
        return env ? env : what.str();
    }
};

struct ConfigVarExpander
{
    ConfigVarExpander(const boost::property_tree::ptree* ptree) : ptree(ptree) {};
    
    std::string operator() (const boost::xpressive::smatch& what) const
    {
        assert(what.size()>1);
        
        try 
        {
            return ptree->get<std::string>((++what.begin())->str().c_str());
        }
        catch(...)
        {
            return what.str();
        }
    }
    
    const boost::property_tree::ptree* ptree;
};
    
void ConfigRoot::parse()
{
    for(auto&& filename : files)
    {
        auto mergeFrom = buildPropertyTree(expandEnvParameters(readConfigFile(filename)));
        mergePropertyTree(ptree,mergeFrom);
    }
    if(!files.empty())
    {
        expandConfigParameters(ptree);
    }
}

std::string ConfigRoot::readConfigFile(const std::string& filename)
{
    std::ifstream configFile(filename);
    return std::string(
        std::istreambuf_iterator<char>(configFile),
        std::istreambuf_iterator<char>());
}

std::string ConfigRoot::expandEnvParameters(const std::string& contents)
{
    using namespace boost::xpressive;
    
    sregex env = "%env." >> (s1 = -+_) >> "%";
    return regex_replace(contents, env, EnvVarExpander());
}

boost::property_tree::ptree ConfigRoot::buildPropertyTree(const std::string& contents)
{
    std::stringstream contentStream;
    contentStream.str(contents);
    boost::property_tree::ptree ptree;
    boost::property_tree::xml_parser::read_xml(contentStream,ptree);
    return ptree;
}

void ConfigRoot::mergePropertyTree(
    boost::property_tree::ptree& mergeTo
  , const boost::property_tree::ptree& mergeFrom)
{
    using namespace boost::property_tree;
            
    auto currMergeTo = mergeTo.ordered_begin();
    auto endMergeTo = mergeTo.not_found();
    
    auto currMergeFrom = mergeFrom.ordered_begin();
    auto endMergeFrom = mergeFrom.not_found();
    
    if(currMergeTo == endMergeTo)
    {
        mergeTo = mergeFrom;
    }
    
    while(currMergeTo != endMergeTo && currMergeFrom != endMergeFrom)
    {
        auto itMergeTo   = mergeTo.to_iterator(currMergeTo);
        auto itMergeFrom = mergeFrom.to_iterator(currMergeFrom);        
        
        if(itMergeTo->first > itMergeFrom->first)
        {
            mergeTo.add_child(itMergeFrom->first,itMergeFrom->second);
            ++currMergeFrom;
            continue;
        }
        
        if(itMergeTo->first == itMergeFrom->first)
        {
            if(itMergeFrom->second.empty())
            {
                mergeTo.put("", itMergeFrom->second.data());
            } 
            mergePropertyTree(itMergeTo->second, itMergeFrom->second);
            ++currMergeFrom;
        }        
        
        if(itMergeTo->first <= itMergeFrom->first)
        {
            ++currMergeTo;
        }
    }
    
    for(;currMergeFrom != endMergeFrom; ++currMergeFrom)
    {
        auto itMergeFrom = mergeFrom.to_iterator(currMergeFrom);
        mergeTo.add_child(itMergeFrom->first,itMergeFrom->second);
    }
}

void ConfigRoot::expandConfigParameters(boost::property_tree::ptree& ptree)
{
    using namespace boost::xpressive;
    
    sregex configParamMatch = "%config." >> (s1 = -+_) >> "%";
    expandConfigParameters(ptree,ptree,configParamMatch);
}

void ConfigRoot::expandConfigParameters(
    const boost::property_tree::ptree& rootNode
  , boost::property_tree::ptree& currNode
  , const boost::xpressive::sregex& match)
{
    using namespace boost::xpressive;
    
    for(auto&& node : currNode)
    { 
        if(node.second.empty())
        {
            std::string value = node.second.get_value<std::string>();
            node.second.put_value(regex_replace(value, match, ConfigVarExpander(&rootNode)));
        }
        else
        {
            expandConfigParameters(rootNode, node.second, match);
        }
    }
}
   
} //namespace detail

std::vector<std::string> configFiles(int argc, char** argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("config,c", po::value<std::vector<std::string>>()->multitoken(), "configuration files");
          
    po::variables_map vm;
    auto parsedOptions = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    po::store(parsedOptions, vm);
    po::notify(vm);    
    
    std::vector<std::string> cfgFiles;
    if (vm.count("help")) 
    {
        std::cout << desc << std::endl;
    }
    else
    if (!vm["config"].empty()) 
    {
        cfgFiles = vm["config"].as<std::vector<std::string>>();
        for(auto& file : cfgFiles)
        {
            std::cout << "Using config file " << file << std::endl;
        }
    }
    else
    {
        std::cout << desc << std::endl;
        std::cout << "Config file not provided" << std::endl;
    }
    
    return cfgFiles;
}
    
Config init(int argc, char** argv)
{
    return config::Config(configFiles(argc,argv));
}

} //namespace config
