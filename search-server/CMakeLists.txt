cmake_minimum_required(VERSION 3.21)
project(cpp_search_server_main)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(cpp_search_server_main
        document.cpp
        document.h
        main.cpp
        paginator.h
        read_input_functions.cpp
        read_input_functions.h
        request_queue.cpp
        request_queue.h
        search_server.cpp
        search_server.h
        string_processing.cpp
        string_processing.h
        test_example_functions.cpp
        test_example_functions.h)
