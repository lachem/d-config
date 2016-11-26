# d-config
A library meant to ease config file reading

## Teaser
```cpp
dconfig::Config config = dconfig::FileFactory({config_cmp1.json, config_cmp2.xml, overrides.json}).create();
dconfig::Config scoped = config.scope("Configuration.Component");
asssert(config.get<std::string>("Configuration.Component.name") == scoped.get<std::string>("name"));
```

## Requirements
* Library requires boost and C++11
* Tests require gtest in gmock

## Building
Make sure BOOST_ROOT environment variable points to boost library directory
* ./configure release
* make
* bin/test

## License
Copyright Adam Lach 2015. Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).
