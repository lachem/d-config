//          Copyright Adam Lach 2022
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//gmock
#include <gmock/gmock.h>

//config
#include <config.hpp>
#include <default_factory.hpp>

//std
#include <string>
#include <unordered_map>

using namespace testing;

namespace test::dconfig {
namespace {

auto config = ::dconfig::DefaultFactory().create({std::string{R"(
{
    "key"    : "value",
    "key2"   : 1234567,
    "array"  : [1,2,4,5],
    "nested" :
    {
        "key"   : "eulav",
        "array" : [9,8,7]
    }
})"}});

struct ConfigVisitorShould : Test {};

TEST_F(ConfigVisitorShould, beCalledWithAllKeyValues)
{
    auto values = std::unordered_map<std::string, std::string>{};
    config.accept([&values](std::string const& key, auto value_or_node) {
        if constexpr (std::is_same_v<decltype(value_or_node), std::string>)
            values.emplace(key, value_or_node);
    });
    EXPECT_FALSE(values.empty());

    EXPECT_TRUE(values.count("key"));
    EXPECT_EQ(std::string{"value"}, values.find("key")->second);

    EXPECT_TRUE(values.count("key2"));
    EXPECT_EQ(std::string{"1234567"}, values.find("key2")->second);
}

TEST_F(ConfigVisitorShould, beCalledWithNestedConfigs)
{
    auto values = std::unordered_map<std::string, ::dconfig::Config>{};
    config.accept([&values](std::string const& key, auto value_or_node) {
        if constexpr (std::is_same_v<decltype(value_or_node), ::dconfig::Config>)
            values.emplace(key, value_or_node);
    });
    EXPECT_FALSE(values.empty());

    EXPECT_TRUE(values.count("array"));
    EXPECT_TRUE(values.count("nested"));
}

TEST_F(ConfigVisitorShould, beCalledForEachElementOfAnArray)
{
    auto values = std::vector<std::pair<std::string, std::string>>{};
    config.scope("array").accept([&values](std::string const& key, auto value_or_node) {
        if constexpr (std::is_same_v<decltype(value_or_node), std::string>)
            values.emplace_back(key, value_or_node);
    });
    EXPECT_EQ(values.size(), 4);

    for (auto const& [key, _] : values)
        EXPECT_EQ(key, std::string{} + ::dconfig::ArrayKey{}.value);

    EXPECT_EQ(values[0].second, std::string{"1"});
    EXPECT_EQ(values[1].second, std::string{"2"});
    EXPECT_EQ(values[2].second, std::string{"4"});
    EXPECT_EQ(values[3].second, std::string{"5"});
}

TEST_F(ConfigVisitorShould, allowForRecursiveCall)
{
    auto values = std::unordered_map<std::string, std::string>{};
    config.accept([&values](std::string const& key, auto value_or_node) {
        if (key != "nested") return;
        if constexpr (std::is_same_v<decltype(value_or_node), ::dconfig::Config>)
            value_or_node.accept([&values](std::string const& key, auto value_or_node) {
                if constexpr (std::is_same_v<decltype(value_or_node), std::string>) {
                    values.emplace(key, value_or_node);
                }
            });
    });

    EXPECT_FALSE(values.empty());

    EXPECT_TRUE(values.count("key"));
    EXPECT_EQ(std::string{"eulav"}, values.find("key")->second);
}

} // namespace
} // namespace test::dconfig
