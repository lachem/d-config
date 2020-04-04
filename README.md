# d-config
A library meant to ease system configuration management supporting:
* xml & json file formats
* configurations distributed across multiple files
* configuration overrides
* configuration scoping
* expanding environment variables
* expanding internal value aliases
* referencing internal config trees
```cpp
dconfig::Config config = dconfig::FileFactory({"config_cmp1.json", "config_cmp2.xml", "overrides.json"}).create();
dconfig::Config scoped = config.scope("Configuration.Component1");
asssert(config.get<std::string>("Configuration.Component1.name") == scoped.get<std::string>("name"));
```

## Table of Contents
- [d-config](#d-config)
  - [Table of Contents](#table-of-contents)
  - [Requirements](#requirements)
  - [Building](#building)
  - [User Guide](#user-guide)
    - [Loading Configuration](#loading-configuration)
    - [Getting Values](#getting-values)
    - [Config Scoping](#config-scoping)
    - [Environment Access](#environment-access)
    - [Value Aliasing](#value-aliasing)
    - [Node Aliasing](#node-aliasing)
    - [Relative Paths](#relative-paths)
    - [Array Support](#array-support)
  - [License](#license)

## Requirements
* Library requires boost and C++11
* Testing requires gtest and gmock

## Building
Make sure BOOST_ROOT environment variable points to boost library directory
* ./configure release
* make
* bin/test

## User Guide

### Loading Configuration
There are three supported ways of building a single ```dconfig::Config``` object.

From text:
```cpp
dconfig::Config config({file1Contets, file2Contents});
```
From files:
```cpp
dconfig::Config config = dconfig::FileFactory({"pathToFile1.json"...}).create();
```
From main params:
```cpp
${prog_name} --config pathToFile1.json pathToFile2.json pathToFile3.xml

int main(int argc, const char* argv[])
{
  dconfig::Config config = dconfig::InitFactory(argc, argv).create();
  ...
```

**Note** | Entries of subsequent config files overwrite corresponding previous config file entries.

### Getting Values
Config class provides two ways of accessing values of a field ```Config::get<T>(path)``` and ```Config::getAll<T>(path)```. Template parameter allows Config to interpret the underlying value(s) as user selected types.

In case of single element value:
```cpp
boost::optional<int32_t> value = config.get<int32_t>("dot.separated.path.to.element");
```
In case of multiple elements with the same name:
```cpp
std::vector<int32_t> value = config.getAll<int32_t>("dot.separated.path.to.repeated.element");
```

### Config Scoping
Config scoping is a feature that allows to create a view (not an actual copy) of a config subset. All paths in the scoped config become relative to the scope. This functionality has been introduced to support configurations for reusable components.
```cpp
dconfig::Config scoped = config.scope("Configuration.Component1");
assert(config.get<std::string>("Configuration.Component1.name") == scoped.get<std::string>("name"));
```
**Note** | Scoping is a cheap operation as the scoped config points to the same internal representation as the original one

### Environment Access
Instead of necessarily providing all the information fixed in the config it is possible to use environment variables that will be filled in by the Config class during building phase. Environment variable syntax ```%env.{name of the variable}%```.
```json
{
  "Config":
  {
    "User" : "%env.USER%",
    "Path" : "%env.PWD%"
  }
}
```

### Value Aliasing
In certain cases it is necessary to repeat same value inside the config in multiple places. To avoid duplication ```dconfig``` supports value aliasing. Any previous (in top down order) tag value can be mentioned in any subsequents tags using a special syntax ```%config.{dot.separted.path.to.property}%```
```json
{
  "Config":
  {
    "User" : "%env.USER%",
    "Host" : "%env.HOSTNAME%"
  },
  "IOComponent":
  {
    "Identification" : "%config.Config.User%_%config.Config.Host%"
  }
}
```
Becomes (given USER=username, HOST=hostname)
```json
{
  "Config":
  {
    "User" : "username",
    "Host" : "hostname"
  },
  "IOComponent":
  {
    "Identification" : "username_hostname"
  }
}
```
**Note** | This functionality is based on text replacement, thus there are no limitations on the number of aliases in one expression

### Node Aliasing
While value aliasing is a useful tool to reference other configuration elements, it is limited to simple properties. When referencing configuration tree nodes is needed, dconfig provides node aliasing feature. In order to make use of it one needs to put ```%node.{dot.separted.path.to.node}%``` as property value as demonstrated below.
```json
{
  "Config":
  {
    "Identification":
    {
      "User" : "username",
      "Host" : "hostname"
    }
  },
  "IOComponent":
  {
    "Identification" : "%node.Config.Identification%"
  }
}
```
Becomes
```json
{
  "Config":
  {
    "Identification":
    {
      "User" : "username",
      "Host" : "hostname"
    }
  },
  "IOComponent":
  {
    "Identification" :
    {
      "User" : "username",
      "Host" : "hostname"
    }
  }
}
```
**Note** | Aliased nodes are references and not copies, thus yielding better runtime performance and more compact representation in memory.

### Relative Paths
Paths to nodes and properties can be provided in two ways, absolute and relative. Aboslute paths require the user to specify complete set of dot separated node names that lead to the referenced property or node (see prior examples). Relative paths instead allow the user to specify referenced property or node in relation to reference position. The number of dots after function specifier (i.e. ```%param``` or ```%node```) denote how many levels up in the configuration tree to go.
```json
{
  "Component" :
  {
    "Config":
    {
      "Identification":
      {
        "User" : "username",
        "Host" : "hostname"
      }
    },
    "IOComponent":
    {
      "Address" :
      {
        "node" : "127.0.0.1",
        "port" : "27800"
      },
      "Destination" : "%config.Address.node%:%config.Address.port%",
      "Identification" : "%node..Config.Identification%",
      "Source" : "%config..Config.Identification.Host%"
    }
  }
}
```
Becomes
```json
{
  "Component" :
  {
    "Config":
    {
      "Identification":
      {
        "User" : "username",
        "Host" : "hostname"
      }
    },
    "IOComponent":
    {
      "Address" :
      {
        "node" : "127.0.0.1",
        "port" : "27800"
      },
      "Destination" : "127.0.0.1:27800",
      "Identification" :
      {
        "User" : "username",
        "Host" : "hostname"
      },
      "Source" : "hostname"
    }
  }
}
```
**Note** | Relative paths take precedence in case of ambiguity.

### Array Support
The api is the same for XML and JSON files with one exception namely arrays. As there is no notion of an array in XML the syntax for array in XML is slightly different than for json and requires the usage of special tag character:
```cpp
std::vector<int32_t> values = config.getAll<int32_t>("Config.Array.");
```
or
```cpp
std::vector<int32_t> values = config.scope("Config.Array").getAll<int32_t>("");
```
Corresponding json file:
```json
{
  "Config":
  {
    "Array": [ 10, 20 ]
  }
}
```
Corresponding xml file:
```xml
<Config>
  <Array>
    <.>10</.>
    <.>10</.>
  </Array>
</Config>
```
Using ```getAll<...>("Config.Element")``` is different from ```getAll<...>("Config.Element.")``` as the former refers to repeated Element tag and the latter to an array under Element tag. Distinguishing the two is important to support repeated node arrays, like in the example below:
```cpp
for (const auto& scope : config.scopes("Config.ManyArrays"))
  auto values = scope.getAll<int>("");
```
Corresponding json file:
```json
{
  "Config":
  {
    "ManyArrays": [ 10, 20 ],
    "ManyArrays": [ 30, 40 ]
  }
}
```

## License
Copyright Adam Lach 2020. Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).
