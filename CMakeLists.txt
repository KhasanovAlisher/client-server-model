cmake_minimum_required(VERSION 3.5)

project(room_rent LANGUAGES C)

set(COMMONS
    z_net_lib/z_net_common.h
    z_net_lib/z_net_common_pvt.h
)

add_executable(server
    server.c
    sql_manipulations.h
    sql_manipulations.c
    z_net_lib/z_net_server.c
    z_net_lib/z_net_server.h
    ${COMMONS}
)

add_executable(client
    client.c
    z_net_lib/z_net_client.c
    z_net_lib/z_net_client.h
    ${COMMONS}
)

target_link_libraries(server pthread json-c sqlite3)
target_link_libraries(client pthread)
