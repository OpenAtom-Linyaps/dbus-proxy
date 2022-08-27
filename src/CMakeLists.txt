aux_source_directory(proxy PROXY_SRC)
aux_source_directory(message MSG_SRC)
aux_source_directory(filter FILTER_SRC)

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
    ${DBUS_LIBRARIES}
    )

add_executable(ll-dbus-proxy
        ${MAIN_SOURCES})

target_link_libraries(ll-dbus-proxy
                      PRIVATE ${LINK_LIBS})

target_include_directories(ll-dbus-proxy PRIVATE ${DBUS_INCLUDE_DIRS})

install(FILES resource/dbus_map_config
DESTINATION ${CMAKE_INSTALL_PREFIX}/share/permission/policy/linglong)

#设置生成目标二进制的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
install(TARGETS ll-dbus-proxy RUNTIME DESTINATION bin)