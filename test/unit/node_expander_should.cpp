//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//gmock
#include <gmock/gmock.h>

//config
#include <config.hpp>
#include <default_builder.hpp>

//std
#include <iostream>
#include <string>

using testing::Test;
using testing::_;

namespace test {
namespace dconfig {
namespace {

struct XmlExtension
{
    static constexpr bool supportsArrays() { return false; }
    static constexpr const char* contents()
    {
        return R"(
            <?xml version='1.0' encoding='UTF-8' standalone='no'?>
            <ConfigShould>
                <Ingredients>
                    <Array>
                        <.>Elem1</.>
                        <.>Elem2</.>
                        <.>Elem3</.>
                    </Array>
                </Ingredients>
                <System>
                    <DataPath>%env.RUNTIME%/data</DataPath>
                    <SessionFile>filename</SessionFile>
                    <SessionId>STH</SessionId>
                    <SessionInstance>20</SessionInstance>
                    <SessionUniqueId>%config.ConfigShould.System.SessionId%-%config.ConfigShould.System.SessionInstance%</SessionUniqueId>
                    <SessionUniqueId2>%config.ConfigShould.System.SessionUniqueId%-2</SessionUniqueId2>
                    <SessionStatus>Disabled</SessionStatus>
                </System>
                <NodeInjected>
                    %node.ConfigShould.System%
                </NodeInjected>
                <ArrayInjected>
                    <.>%node.ConfigShould.Ingredients%</.>
                    <.>%node.ConfigShould.Ingredients%</.>
                </ArrayInjected>
                <Gateway>
                    <Parameters>
                        <Destination>XETRA</Destination>
                        <LinkId>155</LinkId>
                    </Parameters>
                    <Settings>
                        <Link>%node..Parameters%</Link>
                    </Settings>
                    <ParametersAlias>%node.Parameters%</ParametersAlias>
                    <LinkAlias>%node.Settings.Link%</LinkAlias>
                    <LinkArray>
                        <.>
                            <Destination>LSE</Destination>
                            <LinkId>98</LinkId>
                        </.>
                        <.>%node..Settings.Link%</.>
                    </LinkArray>
                </Gateway>
            </ConfigShould>)";
    }
};

struct JsonExtension
{
    static constexpr bool supportsArrays() { return true; }
    static constexpr const char* contents()
    {
        return R"({
            "ConfigShould" :
            {
                "Ingredients" :
                {
                    "Array" :
                    [
                        "Elem1",
                        "Elem2",
                        "Elem3"
                    ]
                },
                "System" :
                {
                    "DataPath"         : "%env.RUNTIME%/data",
                    "SessionFile"      : "filename",
                    "SessionId"        : "STH",
                    "SessionInstance"  : 20,
                    "SessionUniqueId"  : "%config.ConfigShould.System.SessionId%-%config.ConfigShould.System.SessionInstance%",
                    "SessionUniqueId2" : "%config.ConfigShould.System.SessionUniqueId%-2",
                    "SessionStatus"    : "Disabled"
                },
                "NodeInjected" : "%node.ConfigShould.System%",
                "ArrayInjected" :
                [
                    "%node.ConfigShould.Ingredients%",
                    "%node.ConfigShould.Ingredients%"
                ],
                "Gateway"  :
                {
                    "Parameters" :
                    {
                        "Destination" : "XETRA",
                        "LinkId"      : "155"
                    },
                    "Settings" :
                    {
                        "Link" : "%node..Parameters%"
                    },
                    "ParametersAlias" : "%node.Parameters%",
                    "LinkAlias" : "%node.Settings.Link%",
                    "LinkArray" :
                    [
                        {
                            "Destination" : "LSE",
                            "LinkId"      : "98"
                        },
                        "%node..Settings.Link%"
                    ]
                }
            }})";
    }
};

template<typename T>
struct NodeExpanderShould : public Test, public T
{
    void SetUp() override
    {
        config.reset(new ::dconfig::Config(
            ::dconfig::DefaultBuilder().build(
                { std::string(T::contents()) })));
    }

    std::shared_ptr<::dconfig::Config> config;
};

using TestTypes = ::testing::Types<XmlExtension, JsonExtension>;
TYPED_TEST_CASE(NodeExpanderShould, TestTypes);

TYPED_TEST(NodeExpanderShould, expandNodeWithAbsolutePath)
{
    auto scope = this->config->scope("ConfigShould.NodeInjected");

    EXPECT_EQ(std::string("filename")  , scope.template get<std::string>("SessionFile"));
    EXPECT_EQ(std::string("/root/data"), scope.template get<std::string>("DataPath"));
    EXPECT_EQ(std::string("STH-20-2")  , scope.template get<std::string>("SessionUniqueId2"));
    EXPECT_EQ(std::string("Disabled")  , scope.template get<std::string>("SessionStatus"));
}

TYPED_TEST(NodeExpanderShould, supportArrayExpansion)
{
    auto &&scopes = this->config->scope("ConfigShould.ArrayInjected").scopes(".");

    EXPECT_EQ(2, scopes.size());

    for (const auto& scope : scopes)
    {
        auto&& values = scope.template getAll<std::string>("Array.");

        ASSERT_EQ(3, values.size());

        EXPECT_EQ(std::string("Elem1"),values[0]);
        EXPECT_EQ(std::string("Elem2"),values[1]);
        EXPECT_EQ(std::string("Elem3"),values[2]);
    }
}

TYPED_TEST(NodeExpanderShould, expandNodeWithRelativePath)
{
    auto scope = this->config->scope("ConfigShould.Gateway.Settings");

    auto actual = scope.template get<std::string>("Link.Destination");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("XETRA"), *actual);

    actual = scope.template get<std::string>("Link.LinkId");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("155"), *actual);
}

TYPED_TEST(NodeExpanderShould, expandNodeWithRelativeSameLevelPath)
{
    auto scope = this->config->scope("ConfigShould.Gateway.ParametersAlias");

    auto actual = scope.template get<std::string>("Destination");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("XETRA"), *actual);

    actual = scope.template get<std::string>("LinkId");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("155"), *actual);
}

TYPED_TEST(NodeExpanderShould, expandChainOfReferences)
{
    auto scope = this->config->scope("ConfigShould.Gateway.LinkAlias");

    auto actual = scope.template get<std::string>("Destination");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("XETRA"), *actual);

    actual = scope.template get<std::string>("LinkId");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("155"), *actual);
}

TYPED_TEST(NodeExpanderShould, supportMixingNormalAndNodeElementsInArray)
{
    auto scopes = this->config->scopes("ConfigShould.Gateway.LinkArray.");

    ASSERT_EQ(2, scopes.size());

    auto actual = scopes[0].template get<std::string>("Destination");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("LSE"), *actual);

    actual = scopes[0].template get<std::string>("LinkId");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("98"), *actual);

    actual = scopes[1].template get<std::string>("Destination");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("XETRA"), *actual);

    actual = scopes[1].template get<std::string>("LinkId");
    ASSERT_TRUE(static_cast<bool>(actual));
    EXPECT_EQ(std::string("155"), *actual);
}

} // namespace
} // namespace dconfig
} // namespace test

