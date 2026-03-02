cmake_minimum_required(VERSION 3.21)
project(mq++ VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

include(FetchContent)

FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(stduuid
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
    GIT_TAG v1.2.3)
FetchContent_MakeAvailable(stduuid)

set(MQPP_SOURCES
    src/core/message.cpp
        src/client/broker.cpp
    src/core/subscription_manager.cpp

    src/persistence/message_store.cpp

    src/network/http_transport.cpp

    src/client/publisher.cpp
    src/client/consumer.cpp

    src/threading/thread_pool.cpp
        src/dispatching/message_dispatcher.cpp
        examples/consumer_example.cpp
        src/util/time_util.hpp
        src/util/uuid_util.hpp
)

add_library(mqpp ${MQPP_SOURCES})
target_include_directories(mqpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(mqpp PUBLIC
    httplib::httplib
    stduuid
    pthread
)

add_executable(simple_broker examples/broker_example.cpp)
target_link_libraries(simple_broker PRIVATE mqpp)

add_executable(simple_publisher examples/publisher_example.cpp)
target_link_libraries(simple_publisher PRIVATE mqpp)

add_executable(simple_consumer examples/consumer_example.cpp)
target_link_libraries(simple_consumer PRIVATE mqpp)
