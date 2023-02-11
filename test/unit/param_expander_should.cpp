//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//gmock
#include <gmock/gmock.h>

//config
#include <config.hpp>
#include <default_factory.hpp>

//std
#include <iostream>
#include <string>

using testing::Test;
using testing::_;

namespace test::dconfig {
namespace {

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
                        <Name>%param..Parameters.Destination%</Name>
                        <Address>100.200.100.%param..Parameters.LinkId%</Address>
                        <Description>
                            <UniqueName>%param...Parameters.Destination%-%param...Parameters.LinkId%</UniqueName>
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
                <Algo>
                    <IOC>true</IOC>
                    <Limit>%param.IOC%</Limit>
                    <Smoke>%param.Limit%</Smoke>
                </Algo>
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
                        "Name"           : "%param..Parameters.Destination%",
                        "Address"        : "100.200.100.%param..Parameters.LinkId%",
                        "Description"    :
                        {
                            "UniqueName" : "%param...Parameters.Destination%-%param...Parameters.LinkId%"
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
                ],
                "Algo" :
                {
                    "IOC"   : "true",
                    "Limit" : "%param.IOC%",
                    "Smoke" : "%param.Limit%"
                }
            }})";
    }
};

template<typename T>
struct ParamExpanderShould : public Test, public T
{
    void SetUp() override
    {
        config.reset(new ::dconfig::Config(
            ::dconfig::DefaultFactory().create(
                { std::string(T::contents()) })));
    }

    std::shared_ptr<::dconfig::Config> config;
};

using TestTypes = ::testing::Types<XmlExtension, JsonExtension>;
TYPED_TEST_CASE(ParamExpanderShould, TestTypes);

TYPED_TEST(ParamExpanderShould, expandNodesMarkedParam)
{
    auto scope = this->config->scope("Config.Gateway");

    EXPECT_EQ(std::string("None")          , scope.template get<std::string>("Settings.Name"));
    EXPECT_EQ(std::string("100.200.100.-1"), scope.template get<std::string>("Settings.Address"));
    EXPECT_EQ(std::string("None--1")       , scope.template get<std::string>("Settings.Description.UniqueName"));
}

TYPED_TEST(ParamExpanderShould, expandParamExpansionOnTemplates)
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

TYPED_TEST(ParamExpanderShould, expandChainOfReferences)
{
    EXPECT_EQ(std::string("true"), *this->config->template get<std::string>("Config.Algo.IOC"  ));
    EXPECT_EQ(std::string("true"), *this->config->template get<std::string>("Config.Algo.Limit"));
    EXPECT_EQ(std::string("true"), *this->config->template get<std::string>("Config.Algo.Smoke"));
}

} // namespace
} // namespace test::dconfig
