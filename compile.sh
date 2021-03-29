#!/bin/bash
g++-10 -std=c++20 -m64 -c chronicle.cpp -o chronicle.o
g++ -o chronicle chronicle.o -m64


