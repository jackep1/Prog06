#!/bin/bash
g++ -g3 -Wall -std=c++20 main.cpp gl_frontEnd.cpp level_io.cpp PacMan.cpp Ghost.cpp ColorRGB.cpp -lm -lGL -lglut -lpthread -o test
