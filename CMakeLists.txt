cmake_minimum_required(VERSION 3.28)
project(Cplus CXX)

set(CMAKE_CXX_STANDARD 17)

# nlohmann_json を探す
find_package(nlohmann_json REQUIRED)

include_directories(src src/BioHazard)

add_executable(Cplus
        src/main.cpp
        src/BioHazard/Player.cpp
        src/BioHazard/Player.h
        src/BioHazard/Item/Item.cpp
        src/BioHazard/Item/Item.h
        src/BioHazard/Item/Herb/Herb.cpp
        src/BioHazard/Item/Herb/Herb.h
        src/BioHazard/Item/Key/Key.cpp
        src/BioHazard/Item/Key/Key.h
        src/BioHazard/biohazard.h
        src/BioHazard/biohazard.cpp
        src/CommandPattern/OperationManager.cpp
        src/CommandPattern/OperationManager.h
        src/CommandPattern/AppContext.h
        src/CommandPatterFunction/CommandPatternFunction.cpp
        src/CommandPatterFunction/CommandPatternFunction.h
        src/validationLogic/sample.cpp
        src/validationLogic/sample.h
        src/sample4.cpp
        src/HexDump/HexDump.cpp
        src/HexDump/HexDump.h
        src/CalibrationBin/CalibrationBin.cpp
        src/CalibrationBin/CalibrationBin.h
        src/Packed12bit/Packed12bit.cpp
        src/Packed12bit/Packed12bit.h
)

# ライブラリをリンク
target_link_libraries(Cplus PRIVATE nlohmann_json::nlohmann_json)
