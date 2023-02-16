cmake_minimum_required(VERSION 3.24)
project(G1_AfterTerm C)

set(CMAKE_C_STANDARD 11)

add_executable(server src/server.c src/server.h src/debug.h)
add_executable(client src/client.c src/client.h src/debug.h)

link_libraries(ws2_32)
target_link_libraries(server wsock32)

target_link_libraries(client mingw32)
target_link_libraries(client SDL2main)
target_link_libraries(client SDL2)
target_link_libraries(client SDL2_image)
target_link_libraries(client SDL2_ttf)
target_link_libraries(client SDL2_mixer)
target_link_libraries(client wsock32)