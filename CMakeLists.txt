cmake_minimum_required(VERSION 3.15)

add_library(RxECS STATIC 
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
    src/robin_hood.natvis
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
    src/Stream.h
    src/Stream.cpp
    src/EntityHandle.h
    src/EntityHandle.cpp
    src/TableIterator.h
    src/ComponentIterator.h
    src/TableView.h
    src/TableView.cpp
    src/Filter.cpp
    src/Filter.h
    src/TableViewImpl.h
    src/ColumnImpl.h
    src/EntityQueue.h
    src/EntityQueue.cpp
    src/EntityQueueHandle.h
    src/EntityQueueImpl.h
    )

 add_executable(RxECS_tests 
    test/TestMain.cpp
    test/doctest.h
    test/WorldTests.cpp
    test/QueryTests.cpp
    test/TestComponents.h 
    test/SystemTests.cpp 
    test/StreamTests.cpp
    test/ModuleTests.cpp
         test/ThreadTests.cpp)

target_link_libraries(RxECS_tests PRIVATE RxECS)

target_include_directories(RxECS PUBLIC "src")
target_include_directories(RxECS INTERFACE "include")
target_compile_options(RxECS PUBLIC -DNOMINMAX)

if (MSVC)
    target_compile_options(RxECS PUBLIC /W4 /WX -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING -DWIN32_LEAN_AND_MEAN)
endif ()

set_target_properties(RxECS PROPERTIES CXX_STANDARD 20)
set_target_properties(RxECS_tests PROPERTIES CXX_STANDARD 20)

