#!/usr/bin/env bash
printf '\033[0;1;37mBuilding debug build...\033[0m\n'
cmake -S . -Bdebug -DCMAKE_BUILD_TYPE=Debug && cd ./debug/ && make
printf '\033[0;1;37m... done!\033[0m\n'
