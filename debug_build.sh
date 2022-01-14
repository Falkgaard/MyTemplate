#!/usr/bin/env bash
printf '\033[0;1;37mBuilding debug build...\033[0m\n'
cmake -S . -Bdebug -DCMAKE_BUILD_TYPE=Debug && rm -f ./compile_commands.json && ln -s ./debug/compile_commands.json ./ && cd ./debug/ && make -j4 && cd .. && ./dat/shaders/compile.sh
if [ "$?" -ne "0" ]; then
	printf '\033[0;1;37m... error!\033[0m\n'
	exit 1
else
	printf '\033[0;1;37m... done!\033[0m\n'
	exit 0
fi
#eof
