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
#include <string>

using testing::Test;
using testing::_;

namespace test::dconfig {
namespace {

auto config = ::dconfig::DefaultBuilder().build({std::string{R"(
{
    "key"    : "value",
    "array"  : [1,2,4,5],
    "nested" :
    {
        "key"   : "eulav",
        "array" : [9,8,7]
    }
})"}});

struct ConfigVisitorShould : ::testing::Test {};

TEST_F(ConfigVisitorShould, beCool)
{
    // config.accept([](auto node, auto value_or_node) {

    // });
}

} // namespace
} // namespace test::dconfig
