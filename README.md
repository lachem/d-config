# d-config
A library meant to ease system configuration management supporting:
* xml & json file formats
* configurations distributed across multiple files
* configuration overrides
* configuration scoping
* expanding environment variables
* expanding internal value alisases

## Table of Contents
- [Teaser](#teaser)
- [Requirements](#requirements)
- [Building](#building)
- [User Guide](#user_guide)
  - [Loading Configuration](#loading_configuration)
  - [Getting Values](#getting_values)
  - [Config Scoping](#config_scoping)  
  - [Environment Access](#environment_access)
  - [Value Aliasing](#value_aliasing)
  - [Note About Arrays](#note_about_arrays)
- [License](#license)

## Teaser
```cpp
dconfig::Config config = dconfig::FileFactory({"config_cmp1.json", "config_cmp2.xml", "overrides.json"}).create();
dconfig::Config scoped = config.scope("Configuration.Component1");
asssert(config.get<std::string>("Configuration.Component1.name") == scoped.get<std::string>("name"));
```

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
### Getting Values
### Config Scoping
### Environment Access
### Value Aliasing)
### Note About Arrays

## License
Copyright Adam Lach 2015. Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).
