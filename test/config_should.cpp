//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//test tools
#include <gmock/gmock.h>

//config
#include <config.hpp>
#include <init_factory.hpp>
#include <file_factory.hpp>

//std
#include <fstream>

using testing::Test;
using testing::_;

namespace test 
{
namespace dconfig 
{

struct InitConfigLoader
{   
    void loadConfig(const ::dconfig::Config::separator_type& separator = 
            ::dconfig::Config::separator_type()) 
    {
        int argc = 4; 
        char* argv[4];
        argv[0] = (char*)"program_name";
        argv[1] = (char*)"--config";
        argv[2] = (char*)"test/config.xml";
        argv[3] = (char*)"test/config_override.xml";

        config.reset(new ::dconfig::Config(
                    ::dconfig::InitFactory(argc, argv, separator).create()));
    }

    std::shared_ptr<::dconfig::Config> config;
};

struct TextConfigLoader
{   
    void loadConfig(const ::dconfig::Config::separator_type& separator = 
            ::dconfig::Config::separator_type()) 
    {
        std::vector<std::string> files;
        files.push_back(loadFile("test/config.xml"));
        files.push_back(loadFile("test/config_override.xml"));

        config.reset(new ::dconfig::Config(files, separator));
    }

    std::string loadFile(const std::string& filename)
    {
        std::ifstream configFile(filename);
        return std::move(std::string(
            std::istreambuf_iterator<char>(configFile),
            std::istreambuf_iterator<char>()));
    }
    
    std::shared_ptr<::dconfig::Config> config;
};

struct FileConfigLoader
{   
    void loadConfig(const ::dconfig::Config::separator_type& separator = 
            ::dconfig::Config::separator_type()) 
    {
        std::vector<std::string> files;
        files.push_back("test/config.xml");
        files.push_back("test/config_override.xml");
        
        config.reset(new ::dconfig::Config(
                    ::dconfig::FileFactory(files, separator).create()));
    }

    std::shared_ptr<::dconfig::Config> config;
};

template <typename T>
struct ConfigShould : public Test, public T
{
};

typedef ::testing::Types<FileConfigLoader, TextConfigLoader, InitConfigLoader> TestTypes;
TYPED_TEST_CASE(ConfigShould, TestTypes);

TYPED_TEST(ConfigShould, inidicateInitializedState)
{
    this->loadConfig();
    EXPECT_TRUE(static_cast<bool>(*(this->config)));
}

TYPED_TEST(ConfigShould, returnSingleParameters)
{
    this->loadConfig();
    EXPECT_EQ(std::string("filename"), this->config->template get<std::string>("ConfigShould.System.SessionFile"));
}

TYPED_TEST(ConfigShould, expandEnvironmentVariables)
{
    setenv("RUNTIME","/root",1);
    this->loadConfig();
    EXPECT_EQ(std::string("/root/data"), this->config->template get<std::string>("ConfigShould.System.DataPath"));
}

TYPED_TEST(ConfigShould, expandEnvironmentVariables2)
{
    setenv("SOURCE","/source",1);
    this->loadConfig();
    EXPECT_EQ(std::string("/source/data"), this->config->template get<std::string>("ConfigShould.System.FixConfig"));
}

TYPED_TEST(ConfigShould, allowSlashSeperator)
{
    setenv("SOURCE","/source",1);
    this->loadConfig(::dconfig::Config::separator_type("/"));
    EXPECT_EQ(std::string("/source/data"), this->config->template get<std::string>("ConfigShould/System/FixConfig"));
}

TYPED_TEST(ConfigShould, expandConfigParameters)
{
    this->loadConfig();
    EXPECT_EQ(std::string("STH-20"), this->config->template get<std::string>("ConfigShould.System.SessionUniqueId"));
}

TYPED_TEST(ConfigShould, expandConfigParametersUsingPreviouslyExpandedParameters)
{
    this->loadConfig();
    EXPECT_EQ(std::string("STH-20-2"), this->config->template get<std::string>("ConfigShould.System.SessionUniqueId2"));
}

TYPED_TEST(ConfigShould, provideConfigOptionsFromAllFiles)
{
    this->loadConfig();
    EXPECT_EQ(std::string("Undisclosed1"), this->config->template get<std::string>("ConfigShould.Undisclosed.Name"));
    EXPECT_EQ(std::string("Enabled"),       this->config->template get<std::string>("ConfigShould.System.SessionStatus3"));
    EXPECT_EQ(std::string("NestedValue"),   this->config->template get<std::string>("ConfigShould.Quiz2.Nested"));
    EXPECT_EQ(std::string("NestedValue2"),  this->config->template get<std::string>("ConfigShould.Zip.Nested"));
}

TYPED_TEST(ConfigShould, overrideExistingOptions)
{
    this->loadConfig();
    EXPECT_EQ(std::string("Enabled"), this->config->template get<std::string>("ConfigShould.System.SessionStatus"));
}

TYPED_TEST(ConfigShould, useOverridenValuesWhenExpandingConfigParameters)
{
    this->loadConfig();
    EXPECT_EQ(std::string("Enabled"), this->config->template get<std::string>("ConfigShould.System.SessionStatus2"));
}

TYPED_TEST(ConfigShould, supportScoping)
{
    this->loadConfig();
    auto configScope = this->config->scope("ConfigShould.System");
    EXPECT_EQ(std::string("Enabled"), configScope.template get<std::string>("SessionStatus2"));
}

TYPED_TEST(ConfigShould, supportMultiScoping)
{
    this->loadConfig();
    auto configScope = this->config->scope("ConfigShould").scope("System");    
    EXPECT_EQ(std::string("Enabled"), configScope.template get<std::string>("SessionStatus2"));
}

TYPED_TEST(ConfigShould, supportMultipleScopes)
{
    this->loadConfig();
    auto configScopes = this->config->scope("ConfigShould").scopes("Repeated");
    ASSERT_EQ(2,configScopes.size());
    EXPECT_TRUE(configScopes.begin()->template get<std::string>("Rep").get().find("Value") != std::string::npos);    
}

TYPED_TEST(ConfigShould, supportMultipleScopes2)
{
    this->loadConfig();
    auto configScopes = this->config->scopes("ConfigShould.Repeated");
    ASSERT_EQ(2,configScopes.size());
    EXPECT_TRUE(configScopes.begin()->template get<std::string>("Rep").get().find("Value") != std::string::npos);    
}

TYPED_TEST(ConfigShould, returnMultipleParameters)
{
    this->loadConfig();
    auto configScope = this->config->scope("ConfigShould");
    auto values = configScope.template getAll<std::string>("Repeated.Rep");
    ASSERT_EQ(3,values.size());
    EXPECT_EQ(std::string("Value1"),values[0]);
    EXPECT_EQ(std::string("Value2"),values[1]);
    EXPECT_EQ(std::string("Value3"),values[2]);
}

} // namespace dconfig
} // namespace test

