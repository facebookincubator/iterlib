cmake_minimum_required(VERSION 2.6)
project(iterlib)

set(ITERLIB_VERSION "0.1")
set(ITERLIB_VERSION_MAJOR "0")

if (CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp")
endif (CMAKE_COMPILER_IS_GNUCXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse4.2 -Woverloaded-virtual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} \
    -fno-omit-frame-pointer -momit-leaf-frame-pointer -DNDEBUG")

include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package(Threads)
find_package(Gtest)
find_package(Folly)
find_package(Rocksdb)
find_package(BZip2)
find_package(JeMalloc)
find_package(RxCpp)

include_directories(${FOLLY_INCLUDE_DIR})
include_directories(${ROCKSDB_INCLUDE_DIR})
include_directories(${BZIP2_INCLUDE_DIR})
include_directories(${JEMALLOC_INCLUDE_DIR})
include_directories(${RXCPP_INCLUDE_DIR})

set(Boost_NO_SYSTEM_PATHS 1)
set(BOOST_ROOT "/usr/local/boost/1.55.0")
find_package(Boost 1.51 COMPONENTS
             context thread program_options regex system chrono
             filesystem
             REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
# Required for ubuntu 16.04/boost-1.58
add_definitions("-DBOOST_VARIANT_USE_RELAXED_GET_BY_DEFAULT")

set(DSOURCES
  src/Dynamic.cpp
)

# Main library source code
set(SOURCES
  src/Iterator.cpp
  src/WrappedIterator.cpp
  src/LimitIterator.cpp
  src/ReverseIterator.cpp
  src/RandomIterator.cpp
  src/CountIterator.cpp
  src/OrderByIterator.cpp
  src/AndIterator.cpp
  src/OrIterator.cpp
  src/DifferenceIterator.cpp
  src/MergeIterator.cpp
  src/ProjectIterator.cpp
  src/GroupByIterator.cpp
  src/FilterIterator.cpp
  src/Item.cpp
)

if (RXCPP_FOUND)
set(SOURCES ${SOURCES} src/RxQuery.cpp)
endif (RXCPP_FOUND)


add_library(dynamic-static STATIC ${DSOURCES})
add_library(dynamic-shared SHARED ${DSOURCES})
set_target_properties(dynamic-shared PROPERTIES
                      VERSION ${ITERLIB_VERSION}
                      SOVERSION ${ITERLIB_VERSION_MAJOR})

set_target_properties(dynamic-static PROPERTIES
                      LINKER_LANGUAGE CXX
                      OUTPUT_NAME "dynamic")
set_target_properties(dynamic-shared PROPERTIES
                      LINKER_LANGUAGE CXX
                      OUTPUT_NAME "dynamic")

set_property(TARGET dynamic-static PROPERTY CXX_STANDARD 14)
set_property(TARGET dynamic-shared PROPERTY CXX_STANDARD 14)

add_library(iterlib-static STATIC ${SOURCES})
add_library(iterlib-shared SHARED ${SOURCES})
set_target_properties(iterlib-shared PROPERTIES
                      VERSION ${ITERLIB_VERSION}
                      SOVERSION ${ITERLIB_VERSION_MAJOR})

set_target_properties(iterlib-static PROPERTIES
                      LINKER_LANGUAGE CXX
                      OUTPUT_NAME "iterlib")
set_target_properties(iterlib-shared PROPERTIES
                      LINKER_LANGUAGE CXX
                      OUTPUT_NAME "iterlib")

set_property(TARGET iterlib-static PROPERTY CXX_STANDARD 14)
set_property(TARGET iterlib-shared PROPERTY CXX_STANDARD 14)

set(TESTS
        tests/DynamicTest.cpp
        tests/IteratorTest.cpp
        tests/RocksDBIteratorTest.cpp
        tests/OrderByIteratorTest.cpp
        tests/MergeIteratorTest.cpp
        tests/NestIteratorTest.cpp
        tests/ProjectIteratorTest.cpp
        tests/LetIteratorTest.cpp
        tests/GroupByIteratorTest.cpp
        tests/FilterIteratorTest.cpp
)

if (RXCPP_FOUND)
set(TESTS ${TESTS} tests/RxTest.cpp)
endif (RXCPP_FOUND)

if (BOOST_FOUND)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I ${BOOST_INCLUDE_DIR}")
endif (BOOST_FOUND)

enable_testing()
foreach(sourcefile ${TESTS})
    string(REPLACE ".cpp" "" exename ${sourcefile})
    string(REGEX REPLACE "^((.+)/)+" "" exename ${exename})
    add_executable(${exename}${ARTIFACT_SUFFIX} ${sourcefile})
    add_test(${exename} ${exename}${ARTIFACT_SUFFIX})
    list(APPEND EXES ${exename}${ARTIFACT_SUFFIX})
    target_link_libraries(${exename}${ARTIFACT_SUFFIX} iterlib-shared
                          dynamic-shared gtest glog rt double-conversion
                          ${FOLLY_LIBRARIES}
                          ${FOLLY_BENCHMARK_LIBRARIES}
                          ${ROCKSDB_LIBRARIES}
                          ${BZIP2_LIBRARIES}
                          ${JEMALLOC_LIBRARIES}
                          ${Boost_LIBRARIES}
                          ${CMAKE_THREAD_LIBS_INIT})
    set_property(TARGET ${exename}${ARTIFACT_SUFFIX} PROPERTY CXX_STANDARD 14)
endforeach(sourcefile ${TESTS})

install(TARGETS dynamic-static COMPONENT devel ARCHIVE DESTINATION lib)
install(TARGETS dynamic-shared COMPONENT runtime DESTINATION lib)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/iterlib"
        COMPONENT devel
        DESTINATION include)

install(TARGETS ${EXES} RUNTIME
        DESTINATION bin
        COMPONENT tools)
