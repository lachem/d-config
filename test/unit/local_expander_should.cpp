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

struct XmlExtension
{
    static constexpr bool supportsArrays() { return false; }
    static constexpr const char* contents()
    {
        return R"(
            <Config>
                <Gateway>
                    <Parameters>
                        <Destination>None</Destination>
                        <LinkId>-1</LinkId>
                    </Parameters>
                    <Settings>
                        <Name>%local..Parameters.Destination%</Name>
                        <Address>100.200.100.%local..Parameters.LinkId%</Address>
                        <Description>
                            <UniqueName>%local...Parameters.Destination%-%local...Parameters.LinkId%</UniqueName>
                        </Description>
                    </Settings>
                </Gateway>
                <Gateways>
                    <%template.Config.Gateway%>
                        <Parameters>
                            <Destination>XETRA</Destination>
                            <LinkId>155</LinkId>
                        </Parameters>
                    </%template.Config.Gateway%>
                </Gateways>
                <Gateways>
                   <%template.Config.Gateway%>
                        <Parameters>
                            <Destination>LSE</Destination>
                            <LinkId>156</LinkId>
                        </Parameters>
                   </%template.Config.Gateway%>
                </Gateways>
            </Config>)";
    }
};

struct JsonExtension
{
    static constexpr bool supportsArrays() { return true; }
    static constexpr const char* contents()
    {
        return R"({
            "Config" :
            {
                "Gateway"  :
                {
                    "Parameters" :
                    {
                        "Destination"    : "None",
                        "LinkId"         : "-1"
                    },
                    "Settings" :
                    {
                        "Name"           : "%local..Parameters.Destination%",
                        "Address"        : "100.200.100.%local..Parameters.LinkId%",
                        "Description"    :
                        {
                            "UniqueName" : "%local...Parameters.Destination%-%local...Parameters.LinkId%"
                        }
                    }
                },
                "Gateways" :
                [
                    {
                        "%template.Config.Gateway%" :
                        {
                            "Parameters" :
                            {
                                "Destination" : "XETRA",
                                "LinkId"      : "155"
                            }
                        }
                    },
                    {
                        "%template.Config.Gateway%" :
                        {
                            "Parameters" :
                            {
                                "Destination" : "LSE",
                                "LinkId"      : "156"
                            }
                        }
                    }
                ]
            }})";
    }
};

template<typename T>
struct LocalExpanderShould : public Test, public T
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
TYPED_TEST_CASE(LocalExpanderShould, TestTypes);

TYPED_TEST(LocalExpanderShould, expandNodesMarkedLocal)
{
    auto scope = this->config->scope("Config.Gateway");

    EXPECT_EQ(std::string("None")          , scope.template get<std::string>("Settings.Name"));
    EXPECT_EQ(std::string("100.200.100.-1"), scope.template get<std::string>("Settings.Address"));
    EXPECT_EQ(std::string("None--1")       , scope.template get<std::string>("Settings.Description.UniqueName"));
}

TYPED_TEST(LocalExpanderShould, supportLocalExpansionOnTemplates)
{
    auto path = std::string("Config.Gateways") + (this->supportsArrays() ? "." : "");
    auto scopes = this->config->scopes(path);

    ASSERT_EQ(2, scopes.size());

    auto xetra = scopes[0];
    EXPECT_EQ(std::string("XETRA")          , *xetra.template get<std::string>("Settings.Name"));
    EXPECT_EQ(std::string("100.200.100.155"), *xetra.template get<std::string>("Settings.Address"));
    EXPECT_EQ(std::string("XETRA-155")      , *xetra.template get<std::string>("Settings.Description.UniqueName"));

    auto lse = scopes[1];
    EXPECT_EQ(std::string("LSE")            , *lse.template get<std::string>("Settings.Name"));
    EXPECT_EQ(std::string("100.200.100.156"), *lse.template get<std::string>("Settings.Address"));
    EXPECT_EQ(std::string("LSE-156")        , *lse.template get<std::string>("Settings.Description.UniqueName"));
}

} // namespace dconfig
} // namespace test

