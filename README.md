# MQ++
MQ++ is the message queue implementation in C++

## Features:
- Cross network communication
- Adaptable payload format
- Message durability

## Prerequisites:
- C++ 23
- Internet connection

## Installation
- `cd project`
- `cmake -B build -S . && cmake --build build`

## Running
There are example programs utilizing this library in `examples/`  

First you need to start broker  
`./build/simple_broker`

Then consumer  
`./build/simple_consumer`

Then producer  
`./build/simple_publisher`

