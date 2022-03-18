# find_package(Qt5 REQUIRED COMPONENTS Core DBus Network)
# add dtk log
find_package(DtkCore REQUIRED)
# find_package(PkgConfig REQUIRED)
# libdbus-1-dev 添加dbus api支持
# pkg_search_module(DBUS REQUIRED dbus-1)

aux_source_directory(proxy PROXY_SRC)
aux_source_directory(message MSG_SRC)
aux_source_directory(filter FILTER_SRC)

# libpolkit-qt5-1-dev libpolkit-agent-1-dev
# find_package(PolkitQt5-1)
# include_directories(${DBUS_INCLUDE_DIRS})

set(MAIN_SOURCES
        main.cpp
        ${PROXY_SRC}
        ${FILTER_SRC}
        ${MSG_SRC}
        )

set(LINK_LIBS
    stdc++
    Qt5::Core
    Qt5::Network
    Qt5::DBus
    dtkcore
    ${DBUS_LIBRARIES}
    # PolkitQt5-1::Agent
    )

add_executable(ll-dbus-proxy
        ${MAIN_SOURCES})

target_link_libraries(ll-dbus-proxy
                      PRIVATE ${LINK_LIBS})

target_include_directories(ll-dbus-proxy PRIVATE ${DBUS_INCLUDE_DIRS})

install(FILES resource/dbus_proxy_config
        DESTINATION /deepin/linglong/config)

#设置生成目标二进制的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
install(TARGETS ll-dbus-proxy RUNTIME DESTINATION bin)