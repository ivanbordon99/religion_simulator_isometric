cmake_minimum_required(VERSION 3.20)
project(MarangatuReligionSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(COMMON_SOURCES
    src/IsometricCamera.cpp
    src/Religion.cpp
    src/Cell.cpp
    src/MultiNoiseBiome.cpp
    src/WorldMap.cpp
    src/AdvancementSystem.cpp
    src/ReplicationEngine.cpp
    src/AgglutinationEngine.cpp
    src/Villager.cpp
    src/TileSet.cpp
    src/Humano.cpp
    src/Timeline.cpp
    src/TimeEngine.cpp
    src/EventLine.cpp
    src/Renderer.cpp
    src/UISystem.cpp
    src/SaveSystem.cpp
    src/Game.cpp
)

add_executable(religion_sim src/main.cpp ${COMMON_SOURCES})

if(WIN32)
    target_link_libraries(religion_sim PRIVATE opengl32 gdi32 user32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(religion_sim PRIVATE X11 OpenGL::GL pthread)
endif()

add_executable(test_simulation tests/test_simulation.cpp ${COMMON_SOURCES})

if(WIN32)
    target_link_libraries(test_simulation PRIVATE opengl32 gdi32 user32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(test_simulation PRIVATE X11 OpenGL::GL pthread)
endif()

enable_testing()
add_test(NAME AllSimulationTests COMMAND test_simulation)
