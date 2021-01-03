cmake_minimum_required(VERSION 3.5)

project(room_rent LANGUAGES C)

add_executable(server
    server.c
    z_net_lib/z_net_server.c
    )
add_executable(client
    client.c
    z_net_lib/z_net_client.c
    )

target_link_libraries(server pthread sqlite3)
target_link_libraries(client pthread)
