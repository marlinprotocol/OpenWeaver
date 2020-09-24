<h1 align="center">
  <img height="130" src="https://github.com/marlinprotocol/OpenWeaver/blob/master/img/OpenWeaver_Black.jpg?raw=true"/>
</h1>

OpenWeaver is a framework that provides components to easily deploy a high-performance relay network across a global cluster of nodes. Design goals include:

- Speed - Latency optimized relaying
- Efficiency - Low resource consumption
- Blockchain-agnostic - Supports multiple blockchains in the same core network
- Multilingual - Supports projects written in different languages
- Streamlined - Easy to deploy and manage

## Cloning

The repository has submodules, so be sure to clone recursively using
```
$ git clone --recurse-submodules https://github.com/marlinprotocol/OpenWeaver.git
```

Alternatively, you can checkout submodules after cloning as well using
```
$ git submodule update --init --recursive
```

## Building

Prerequisites: cmake >= 3.13, autoconf, automake, libtool, g++/clang++ (must support C++17, newer versions preferable)

The codebase is a standard C++/CMake project.
```sh
$ mkdir build && cd build  # to create a build directory

$ cmake .. -DCMAKE_BUILD_TYPE=Release && make -j8  # to build
```

## Notable binaries

After building, you can find the following binaries in the build directory:
- `./beacon/beacon` - Beacon
- `./relay/eth_relay` - Relay
- `./goldfish/goldfish` - Toy network for local testing
- `./multicastsdk/msggen` - Message flooding tool for testing/benchmarking

## Integrations

- Ethereum - A bridge between Ethereum nodes and the core network is built at `./integrations/eth/onramp_eth`

## Running locally

After building,
1. Set up a beacon - `./beacon/beacon`
2. Set up a relay - `./relay/eth_relay "127.0.0.1:8002" "127.0.0.1:8003"`

You now have a local network running. In a production setting, you would want to run these in different instances as a globally distributed network.

## Testing

OpenWeaver has a test suite that's integrated with CTest. After building, tests can be run using
```
$ make test
```
or
```
$ ctest
```

## Support

| Topic  | Link  |
|---|---|
|Development   | <a href="https://discord.gg/pdQZyyy">Discord</a>  |
|Research   | <a href="https://research.marlin.pro">Discourse</a>  |
|Bug(s)  | <a href="https://github.com/marlinprotocol/OpenWeaver/issues/new">Issues</a>  |
|Memes  | <a href="https://t.me/marlinprotocol">Telegram</a>  |
|Spam  | info@marlin.pro  |

## License

Copyright (c) 2020 Marlin Contributors. Licensed under the MIT license. See LICENSE for more information.

Includes third party software in the vendor directory with their own licenses.
