//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include "init.hpp"
#include "file_factory.hpp"

//boost
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace dconfig 
{

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
    return FileFactory(configFiles(argc,argv)).create();
}

} //namespace dconfig

