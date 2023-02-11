//          Copyright Adam Lach 2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//local
#include <config_node.hpp>
#include <config_builder.hpp>
#include <default_builder.hpp>
#include <env_var_expander.hpp>
#include <config_param_expander.hpp>
#include <config_node_expander.hpp>
#include <config_template_expander.hpp>

namespace dconfig {

Config DefaultFactory::create(const std::vector<std::string>& contents) const
{
    auto copy = contents;
    return this->create(std::move(copy));
}

Config DefaultFactory::create(std::vector<std::string>&& contents) const
{
    auto sep = separator;
    return ConfigBuilder(separator, arrayKey)
        .addPreExpander([](std::string& content) { EnvVarExpander().operator()(content); })
        .addPostExpander([sep](detail::ConfigNode& node) { ConfigParamExpander(sep, "config").operator()(node); })
        .addPostExpander([sep](detail::ConfigNode& node) { ConfigTemplateExpander(sep, "template").operator()(node); })
        .addPostExpander([sep](detail::ConfigNode& node) { ConfigParamExpander(sep, "param").operator()(node); })
        .addPostExpander([sep](detail::ConfigNode& node) { ConfigNodeExpander(sep, "node").operator()(node); })
        .build(std::move(contents));
}

} //namespace dconfig

