cmake_minimum_required(VERSION 3.15)

add_library(ECS STATIC 
    src/Table.h
    src/Column.h
    src/Entity.h
    src/Component.h
    src/World.h
    src/World.cpp
    src/Hasher.h
    src/Column.cpp
    src/Table.cpp
    src/robin_hood.h
    src/ArchetypeManager.h
    src/Query.h
    src/QueryResult.h
    src/QueryResult.cpp 
    src/System.h
    src/SystemImpl.h
    src/System.cpp 
    src/SystemBuilder.h
    src/QueryBuilder.h 
    src/QueryImpl.h
    src/SystemBuilder.cpp
)

 add_executable(ECS_tests 
    test/TestMain.cpp
    test/doctest.h
    test/WorldTests.cpp
    test/QueryTests.cpp
    test/TestComponents.h 
    test/SystemTests.cpp)

target_link_libraries(ECS_tests PRIVATE ECS)

target_include_directories(ECS PUBLIC "src")
target_compile_options(ECS PUBLIC -DNOMINMAX)

if (MSVC)
    target_compile_options(ECS PUBLIC /W4 /WX -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING -DWIN32_LEAN_AND_MEAN)
endif ()

set_target_properties(ECS PROPERTIES CXX_STANDARD 20)
set_target_properties(ECS_tests PROPERTIES CXX_STANDARD 20)

