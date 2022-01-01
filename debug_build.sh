#!/usr/bin/env bash
printf '\033[0;1;37mBuilding debug build...\033[0m\n'
cmake -S . -Bdebug -DCMAKE_BUILD_TYPE=Debug && rm -f ./compile_commands.json && ln -s ./debug/compile_commands.json ./ && cd ./debug/ && make && cd .. && ./dat/shaders/compile.sh
printf '\033[0;1;37m... done!\033[0m\n'
#eof
