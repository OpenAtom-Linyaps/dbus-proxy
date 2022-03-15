# 开启GTest开关
enable_testing()

find_package(GTest REQUIRED)

# include_directories(${DBUS_INCLUDE_DIRS})

set(LINK_LIBS
    GTest::GTest
    GTest::Main
    Qt5::Core
    Qt5::Network
    Qt5::DBus
    stdc++
    ${DBUS_LIBRARIES}
)

aux_source_directory(${PROJECT_SOURCE_DIR}/src/proxy PROXY_SRC)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/message MSG_SRC)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/filter FILTER_SRC)

set(GTEST_SOURCES
        dbus_filter_test.cpp
        dbus_message_test.cpp
        ${PROXY_SRC}
        ${FILTER_SRC}
        ${MSG_SRC}
        )

# CMake　相对路径

add_executable(dbus-proxy-test ${GTEST_SOURCES})

target_link_libraries(dbus-proxy-test PRIVATE ${LINK_LIBS})

target_include_directories(dbus-proxy-test PRIVATE ${DBUS_INCLUDE_DIRS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_test(NAME dbus-proxy-test COMMAND dbus-proxy-test)