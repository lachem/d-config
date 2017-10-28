//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//test tools
#include <gmock/gmock.h>

//config
#include <config.hpp>
#include <default_builder.hpp>

//std
#include <fstream>
#include <iostream>
#include <chrono>

namespace test {
namespace dconfig {

struct Performance : ::testing::Test
{
    void load(const std::string& filename)
    {
        std::ifstream configFile(filename);
        auto contents = std::string(
            std::istreambuf_iterator<char>(configFile),
            std::istreambuf_iterator<char>());

        files.push_back(contents);
    }

    template<typename Functor>
    void measure(Functor func, const int repetitions, const std::string& text = std::string())
    {
        using namespace std::chrono;

        auto start = high_resolution_clock::now();
        for (auto i = 0; i<repetitions; ++i)
        {
            asm("");
            auto result = func();
            keepalive = &result;
            asm("");
        }
        auto end = high_resolution_clock::now();
        auto total = duration_cast<nanoseconds>(end-start).count();

        std::cout << "For " << repetitions << " repetitions";
        if (!text.empty())
            std::cout << " and " << text;
        std::cout << std::endl;
        std::cout << "Total time: "   << total << "ns" << std::endl;
        std::cout << "Average time: " << total/repetitions << "ns" << std::endl;
    }

    ::dconfig::Config build(const ::dconfig::Separator& separator = '.')
    {
        return ::dconfig::DefaultBuilder(separator).build(files);
    }

private:
    std::vector<std::string> files;
    void* keepalive;
};

TEST_F(Performance, withMultilevelScoping)
{
    this->load(std::string("test/config.json"));

    auto config = this->build();
    this->measure(
        [&config](){return config.scope("ConfigShould.System");},
        100000);
}

TEST_F(Performance, withCrossLevelValueRetrieval)
{
    this->load(std::string("test/config.json"));

    auto config = this->build();
    this->measure(
        [&config](){return config.get<std::string>("ConfigShould.System.SessionFile");},
        100000);
}

TEST_F(Performance, withCrossLevelIntegerRetrieval)
{
    this->load(std::string("test/config.json"));

    auto config = this->build();
    this->measure(
        [&config](){return config.get<int>("ConfigShould.System.SessionInstance");},
        100000);
}

// -------------------------------------------------------------------
struct PerformanceParameterized : Performance, ::testing::WithParamInterface<const char*>
{
};

TEST_P(PerformanceParameterized, withReplacing)
{
    this->load(std::string("test/config.") + GetParam());

    this->measure(
        [this](){return this->build();},
        10000, std::string(GetParam()) + " file");
}

TEST_P(PerformanceParameterized, withMergingAndReplacing)
{
    this->load(std::string("test/config.") + GetParam());
    this->load(std::string("test/config_override.") + GetParam());

    this->measure(
        [this](){return this->build();},
        10000, std::string(GetParam()) + " file");
}

INSTANTIATE_TEST_CASE_P(ForType, PerformanceParameterized, ::testing::Values("xml", "json"));

} // namespace dconfig
} // namespace test

