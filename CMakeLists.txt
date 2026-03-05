cmake_minimum_required(VERSION 4.1)
project(Cplus CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(Cplus/src)

add_executable(Cplus
        Cplus/src/main.cpp
        Cplus/src/Player.cpp
        Cplus/src/Player.h
        Cplus/src/Item/Item.cpp
        Cplus/src/Item/Item.h
        Cplus/src/Item/Herb/Herb.cpp
        Cplus/src/Item/Herb/Herb.h
        Cplus/src/Item/Key/Key.cpp
        Cplus/src/Item/Key/Key.h
)
