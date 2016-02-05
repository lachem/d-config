//          Copyright Adam Lach 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//test tools
#include <gmock/gmock.h>

//config
#include <config.hpp>

using testing::Test;
using testing::_;

namespace test 
{
namespace config 
{
    
struct ConfigShould : public Test
{
    void loadConfig() 
    {
        std::vector<std::string> files;
        files.push_back("test/config.xml");
        files.push_back("test/config_override.xml");
        config.reset(new ::config::Config(files));
    }

    void loadConfig(const ::config::Config::separator_type& separator) 
    {
        std::vector<std::string> files;
        files.push_back("test/config.xml");
        files.push_back("test/config_override.xml");
        config.reset(new ::config::Config(files, separator));
    }

    std::shared_ptr<::config::Config> config;
};

TEST_F(ConfigShould, returnSingleParameters)
{
    loadConfig();
    EXPECT_EQ(std::string("filename"), config->get<std::string>("ConfigShould.System.SessionFile"));
}

TEST_F(ConfigShould, expandEnvironmentVariables)
{
    setenv("RUNTIME","/root",1);
    loadConfig();
    EXPECT_EQ(std::string("/root/data"), config->get<std::string>("ConfigShould.System.DataPath"));
}

TEST_F(ConfigShould, expandEnvironmentVariables2)
{
    setenv("SOURCE","/source",1);
    loadConfig();
    EXPECT_EQ(std::string("/source/data"), config->get<std::string>("ConfigShould.System.FixConfig"));
}

TEST_F(ConfigShould, allowSlashSeperator)
{
    setenv("SOURCE","/source",1);
    loadConfig(::config::Config::separator_type("/"));
    EXPECT_EQ(std::string("/source/data"), config->get<std::string>("ConfigShould/System/FixConfig"));
}

TEST_F(ConfigShould, expandConfigParameters)
{
    loadConfig();
    EXPECT_EQ(std::string("STH-20"), config->get<std::string>("ConfigShould.System.SessionUniqueId"));
}

TEST_F(ConfigShould, expandConfigParametersUsingPreviouslyExpandedParameters)
{
    loadConfig();
    EXPECT_EQ(std::string("STH-20-2"), config->get<std::string>("ConfigShould.System.SessionUniqueId2"));
}

TEST_F(ConfigShould, provideConfigOptionsFromAllFiles)
{
    loadConfig();
    EXPECT_EQ(std::string("Undisclosed1"), config->get<std::string>("ConfigShould.Undisclosed.Name"));
    EXPECT_EQ(std::string("Enabled"),       config->get<std::string>("ConfigShould.System.SessionStatus3"));
    EXPECT_EQ(std::string("NestedValue"),   config->get<std::string>("ConfigShould.Quiz2.Nested"));
    EXPECT_EQ(std::string("NestedValue2"),  config->get<std::string>("ConfigShould.Zip.Nested"));
}

TEST_F(ConfigShould, overrideExistingOptions)
{
    loadConfig();
    EXPECT_EQ(std::string("Enabled"), config->get<std::string>("ConfigShould.System.SessionStatus"));
}

TEST_F(ConfigShould, useOverridenValuesWhenExpandingConfigParameters)
{
    loadConfig();
    EXPECT_EQ(std::string("Enabled"), config->get<std::string>("ConfigShould.System.SessionStatus2"));
}

TEST_F(ConfigShould, supportScoping)
{
    loadConfig();
    auto configScope = config->scope("ConfigShould.System");
    EXPECT_EQ(std::string("Enabled"), configScope.get<std::string>("SessionStatus2"));
}

TEST_F(ConfigShould, supportMultiScoping)
{
    loadConfig();
    auto configScope = config->scope("ConfigShould").scope("System");    
    EXPECT_EQ(std::string("Enabled"), configScope.get<std::string>("SessionStatus2"));
}

TEST_F(ConfigShould, supportMultipleScopes)
{
    loadConfig();
    auto configScopes = config->scope("ConfigShould").scopes("Repeated");
    ASSERT_EQ(2,configScopes.size());
    EXPECT_TRUE(configScopes.begin()->get<std::string>("Rep").get().find("Value") != std::string::npos);    
}

TEST_F(ConfigShould, supportMultipleScopes2)
{
    loadConfig();
    auto configScopes = config->scopes("ConfigShould.Repeated");
    ASSERT_EQ(2,configScopes.size());
    EXPECT_TRUE(configScopes.begin()->get<std::string>("Rep").get().find("Value") != std::string::npos);    
}

TEST_F(ConfigShould, returnMultipleParameters)
{
    loadConfig();
    auto configScope = config->scope("ConfigShould");
    auto values = configScope.getAll<std::string>("Repeated.Rep");
    ASSERT_EQ(3,values.size());
    EXPECT_EQ(std::string("Value1"),values[0]);
    EXPECT_EQ(std::string("Value2"),values[1]);
    EXPECT_EQ(std::string("Value3"),values[2]);
}

} // namespace config
} // namespace test
