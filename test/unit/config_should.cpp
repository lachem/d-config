//          Copyright Adam Lach 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//gmock
#include <gmock/gmock.h>

//config
#include <config.hpp>
#include <default_builder.hpp>
#include <init_factory.hpp>
#include <file_factory.hpp>

//std
#include <fstream>

using testing::Test;
using testing::_;

namespace test {
namespace dconfig {

struct XmlExtension
{
    constexpr static const char* name = "xml";
};

struct JsonExtension
{
    constexpr static const char* name = "json";
};

template<typename Extension>
struct InitConfigLoader
{
    using extension = Extension;

    void loadConfig(const ::dconfig::Separator& separator = ::dconfig::Separator())
    {
        auto configFile         = std::string("test/config.") + Extension::name;
        auto configFileOverride = std::string("test/config_override.") + Extension::name;
        auto arrayFile          = std::string("test/array.") + Extension::name;

        int argc = 5;
        char* argv[5];
        argv[0] = (char*)"program_name";
        argv[1] = (char*)"--config";
        argv[2] = (char*)configFile.c_str();
        argv[3] = (char*)configFileOverride.c_str();
        argv[4] = (char*)arrayFile.c_str();

        config.reset(new ::dconfig::Config(::dconfig::InitFactory(argc, argv, separator).create()));
    }

    std::shared_ptr<::dconfig::Config> config;
};

template<typename Extension>
struct TextConfigLoader
{
    using extension = Extension;

    void loadConfig(const ::dconfig::Separator& separator = ::dconfig::Separator())
    {
        std::vector<std::string> files;
        files.push_back(loadFile(std::string("test/config.") + Extension::name));
        files.push_back(loadFile(std::string("test/config_override.") + Extension::name));
        files.push_back(loadFile(std::string("test/array.") + Extension::name));

        config.reset(new ::dconfig::Config(::dconfig::DefaultBuilder(separator).build(std::move(files))));
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

template<typename Extension>
struct FileConfigLoader
{
    using extension = Extension;

    void loadConfig(const ::dconfig::Separator& separator = ::dconfig::Separator())
    {
        std::vector<std::string> files;
        files.push_back(std::string("test/config.") + Extension::name);
        files.push_back(std::string("test/config_override.") + Extension::name);
        files.push_back(std::string("test/array.") + Extension::name);

        config.reset(new ::dconfig::Config(::dconfig::FileFactory(files, separator).create()));
    }

    std::shared_ptr<::dconfig::Config> config;
};

template <typename T>
struct ConfigShould : public Test, public T
{
};

using TestTypes = ::testing::Types<
    FileConfigLoader<XmlExtension>,  TextConfigLoader<XmlExtension>,  InitConfigLoader<XmlExtension>,
    FileConfigLoader<JsonExtension>, TextConfigLoader<JsonExtension>, InitConfigLoader<JsonExtension>>;
TYPED_TEST_CASE(ConfigShould, TestTypes);

TYPED_TEST(ConfigShould, inidicateInitializedState)
{
    this->loadConfig();
    EXPECT_TRUE(static_cast<bool>(*(this->config)));
}

TYPED_TEST(ConfigShould, returnSingleParameters)
{
    this->loadConfig();

    auto actual = this->config->template get<std::string>("ConfigShould.System.SessionFile");
    EXPECT_EQ(std::string("filename"), *actual);
}

TYPED_TEST(ConfigShould, returnSingleParametersByRef)
{
    this->loadConfig();
    EXPECT_EQ(std::string("filename"), this->config->getRef("ConfigShould.System.SessionFile")[0]);
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
    this->loadConfig(::dconfig::Separator('/'));
    EXPECT_EQ(std::string("/source/data"), this->config->template get<std::string>("ConfigShould/System/FixConfig"));
}

TYPED_TEST(ConfigShould, expandConfigParameters)
{
    this->loadConfig();

    auto expected = std::string("STH-20");
    auto actual = this->config->template get<std::string>("ConfigShould.System.SessionUniqueId");

    ASSERT_TRUE(actual);
    EXPECT_EQ(expected, *actual);
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

TYPED_TEST(ConfigShould, supportJsonArrays)
{
    this->loadConfig();

    auto scope = this->config->scope("ConfigShould");
    auto&& values = scope.template getAll<std::string>("Array.");

    ASSERT_EQ(3, values.size());

    EXPECT_EQ(std::string("Elem1"),values[0]);
    EXPECT_EQ(std::string("Elem2"),values[1]);
    EXPECT_EQ(std::string("Elem3"),values[2]);
}

TYPED_TEST(ConfigShould, supportJsonArrays2)
{
    this->loadConfig();

    auto scope = this->config->scope("ConfigShould.Array");
    auto&& values = scope.template getAll<std::string>(".");

    ASSERT_EQ(3, values.size());

    EXPECT_EQ(std::string("Elem1"),values[0]);
    EXPECT_EQ(std::string("Elem2"),values[1]);
    EXPECT_EQ(std::string("Elem3"),values[2]);
}

TYPED_TEST(ConfigShould, supportArraysByRef)
{
    this->loadConfig();

    auto scope = this->config->scope("ConfigShould");
    const auto& values = scope.getRef("Array.");

    ASSERT_EQ(3, values.size());

    EXPECT_EQ(std::string("Elem1"),values[0]);
    EXPECT_EQ(std::string("Elem2"),values[1]);
    EXPECT_EQ(std::string("Elem3"),values[2]);
}

TYPED_TEST(ConfigShould, provideValuesByRefFromInternalRepresentation)
{
    this->loadConfig();

    auto overwrite = "Overwritten";
    auto scope = this->config->scope("ConfigShould");

    // force modify internal representation
    {
        const auto& values = scope.getRef("Array.");
        (const_cast<std::vector<std::string>&>(values))[1] = overwrite;
    }

    const auto& values = scope.getRef("Array.");
    ASSERT_EQ(3, values.size());

    EXPECT_EQ(std::string("Elem1"),values[0]);
    EXPECT_EQ(overwrite           ,values[1]);
    EXPECT_EQ(std::string("Elem3"),values[2]);
}

TYPED_TEST(ConfigShould, supportTemplates)
{
    this->loadConfig();

    auto scope = this->config->scope("ConfigShould.Templated");

    EXPECT_EQ(std::string("filename")  , scope.template get<std::string>("SessionFile"));
    EXPECT_EQ(std::string("/root/data"), scope.template get<std::string>("DataPath"));
    EXPECT_EQ(std::string("STH-20-2")  , scope.template get<std::string>("SessionUniqueId2"));
    EXPECT_EQ(std::string("Unknown")   , scope.template get<std::string>("SessionStatus"));
    EXPECT_EQ(std::string("500")       , scope.template get<std::string>("SessionInstance"));
}

TYPED_TEST(ConfigShould, supportBooleanValues)
{
    this->loadConfig();

    EXPECT_TRUE(*this->config->template get<bool>("ConfigShould.BooleanTrue"));
    EXPECT_FALSE(*this->config->template get<bool>("ConfigShould.BooleanFalse"));
}

TYPED_TEST(ConfigShould, supportIntegerBooleanValues)
{
    this->loadConfig();

    EXPECT_TRUE(*this->config->template get<bool>("ConfigShould.IntTrue"));
    EXPECT_FALSE(*this->config->template get<bool>("ConfigShould.IntFalse"));
}

TYPED_TEST(ConfigShould, rethrowBadCastAsInvalidArgument)
{
    this->loadConfig();

    EXPECT_THROW(this->config->template get<int>("ConfigShould.System.SessionFile"), std::invalid_argument);
}

TYPED_TEST(ConfigShould, rethrowBadCastAsInvalidArgument2)
{
    this->loadConfig();

    EXPECT_THROW(this->config->template getAll<int>("ConfigShould.System.SessionFile"), std::invalid_argument);
}

// TYPED_TEST(ConfigShould, getValuesWithAlternatives)
// {
//     this->loadConfig();
//     auto expected = std::string("Enabled");
//     auto actual = this->config->template get<std::string>(
//         alternatives(
//             "ConfigShould.non-existent",
//             "ConfigShould.System.non-existent",
//             "ConfigShould.System.SessionStatus"
//         )
//     );

//     EXPECT_EQ(expected, actual);
// }


} // namespace dconfig
} // namespace test

