cmake_minimum_required(VERSION 3.30)
project(Version3)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Version3
    ColorRGB.cpp
    ColorRGB.h
    Direction.h
    Ghost.cpp
    Ghost.h
    gl_frontEnd.cpp
    gl_frontEnd.h
    glPlatform.h
    level_io.cpp
    main.cpp
    PacMan.cpp
    PacMan.h)
