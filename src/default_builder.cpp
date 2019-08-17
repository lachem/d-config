//          Copyright Adam Lach 2017
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

Config DefaultBuilder::build(const std::vector<std::string>& contents) const
{
    auto copy = contents;
    return this->build(std::move(copy));
}

Config DefaultBuilder::build(std::vector<std::string>&& contents) const
{
    auto sep = separator;
    ConfigBuilder builder(
        [](std::string& content) { EnvVarExpander().operator()(content); },
        [sep](detail::ConfigNode& node) { ConfigParamExpander(sep, "config").operator()(node); },
        [sep](detail::ConfigNode& node) { ConfigTemplateExpander(sep, "template").operator()(node); },
        [sep](detail::ConfigNode& node) { ConfigParamExpander(sep, "param").operator()(node); },
        [sep](detail::ConfigNode& node) { ConfigNodeExpander(sep, "node").operator()(node); },
        separator,
        arrayKey);
    return builder.build(std::move(contents));
}

} //namespace dconfig

