#!/usr/bin/env bash
printf '\033[0;1;37mRunning debug build...\033[0m\n' ;
cd ./debug/ ;
./MyTemplate && printf '\033[0;1;37m... done!\033[0m\n'
#eof
