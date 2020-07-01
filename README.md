<h1 align="center">
  <img height="130" src="https://github.com/marlinprotocol/OpenWeaver/blob/master/img/OpenWeaver_Black.jpg?raw=true"/>
</h1>

# FIBRE for altcoins

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

The codebase is a standard C++/CMake project.
```sh
$ mkdir build && cd build  # to create a build directory

$ cmake .. -DCMAKE_BUILD_TYPE=Release && make -j8  # to build
```

## Notable binaries

After building, you can find the following binaries in the build directory:
- `./beacon/server` - Beacon server
- `./relay/masterexec` - Master node
- `./relay/relayexec` - Relay node
- `./goldfish/goldfish` - Toy network for local testing
- `./multicastsdk/msggen` - Message flooding tool for testing/benchmarking

## Integrations

- Ethereum - A bridge between Ethereum nodes and the core network is built at `./integrations/eth/onramp_eth`

## Testing

OpenWeaver has a test suite that's integrated with CTest. After building, tests can be run using
```
$ make test
```
or
```
$ ctest
```
