#!/usr/bin/env bash
printf '\n \033[0;1;37mBuilding debug build...\033[0m\n'
cmake -S . -Bdebug -DCMAKE_BUILD_TYPE=debug && cd ./debug/ && make
printf '\n \033[0;1;37... done!\033[0m\n'
