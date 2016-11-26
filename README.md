# d-config
A library meant to ease system configuration management supporting:
* xml & json file formats
* configurations distributed across multiple files
* configuration overrides
* configuration scoping
* expanding environment variables
* expanding internal value alisases
```cpp
dconfig::Config config = dconfig::FileFactory({"config_cmp1.json", "config_cmp2.xml", "overrides.json"}).create();
dconfig::Config scoped = config.scope("Configuration.Component1");
asssert(config.get<std::string>("Configuration.Component1.name") == scoped.get<std::string>("name"));
```

## Table of Contents
- [Teaser](#teaser)
- [Requirements](#requirements)
- [Building](#building)
- [User Guide](#user-guide)
  - [Loading Configuration](#loading-configuration)
  - [Getting Values](#getting-values)
  - [Config Scoping](#config-scoping)  
  - [Environment Access](#environment-access)
  - [Value Aliasing](#value-aliasing)
  - [Note About Arrays](#note-about-arrays)
- [License](#license)

## Requirements
* Library requires boost and C++11
* Tests require gtest in gmock

## Building
Make sure BOOST_ROOT environment variable points to boost library directory
* ./configure release
* make
* bin/test

## User Guide
### Loading Configuration
There are three supported ways of building a single Config object.

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

** Note** | Entries of subsequent config files overwrite corresponding (same path) previous config file entries.

### Getting Values
Config class provides two ways of accessisng values of a field ```Config::get<T>(path)``` and ```Config::getAll<T>(path)```. Template parameter allows Config to interpret the underlying value(s) as user selected types.

In case of single element value:
```cpp
boost::optional<int32_t> value = confg.get<int32_t>("dot.separated.path.to.element");
```
In case of multiple elements with the same name:
```cpp
std::vector<int32_t> = confg.getAll<int32_t>("dot.separated.path.to.repeated.element");
```
### Config Scoping
Config scoping is a feature that allows to create a view (not an actual copy) of a config subset. All paths in the scoped config become relative to the scope. This functionality has been introduced to support configurations for reusable components.
```cpp
dconfig::Config scoped = config.scope("Configuration.Component1");
assert(config.get<std::string>("Configuration.Component1.name") == scoped.get<std::string>("name"));
```
**Note** | Scoping is a cheap operation as the scoped config points to the same internal representation as the original one

### Environment Access
Instead of necessarily providing all the information fixed in the config it is possible to use environment variables that will be filled in by the Config class during the building phase. Env. variable syntax ```%env.{name of the variable}%```.
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
In certain cases it is necessary to repeat same value inside the config in multiple places. To avoid duplication dconfig supports value aliasing. Any previous (in top down order) tag value can be mentioned in any subsequents tags using a special syntax ```%config.{dot.separted.path.to.node}%```
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
**Note** | This functionality is based on text replacement, therefore there are no limitations on the number of aliases in one expression 

### Note About Arrays
The api is the same for XML and JSON files with one exception, namely arrays. As there is no notion of an array in XML therefore the following syntax will work only for JSON files:
```cpp
std::vector<int32_t> = confg.getAll<int32_t>("Config.Array.");
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
**Note** | Using ```getAll<...>("Config.Element")``` is different from ```getAll<...>("Config.Element.")``` as the former refers to repeated Element tag and the latter to an array.

## License
Copyright Adam Lach 2015. Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).
