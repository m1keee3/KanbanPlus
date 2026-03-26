cmake_minimum_required(VERSION 3.20)
project(KanbanPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS
        uuid
        system)
find_package(libpqxx CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

set(SERVICE_SOURCES
    src/core/BoardService.cpp
)

add_executable(KanbanPlus
    main.cpp
    ${SERVICE_SOURCES}
        src/storage/postgres/BoardStorage.cpp
)
target_include_directories(KanbanPlus PRIVATE src)
target_link_libraries(KanbanPlus PRIVATE
    Boost::uuid
    Boost::system
    libpqxx::pqxx
)

add_executable(KanbanPlusTests
    tests/BoardServiceTests.cpp
    ${SERVICE_SOURCES}
    src/storage/inmemory/BoardStorage.cpp
)
target_include_directories(KanbanPlusTests PRIVATE src)
target_link_libraries(KanbanPlusTests PRIVATE
    Boost::system
    GTest::gtest_main
)

enable_testing()
add_test(NAME BoardServiceTests COMMAND KanbanPlusTests)
