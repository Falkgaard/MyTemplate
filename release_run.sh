#!/usr/bin/env bash
printf '\033[0;1;37mRunning release build...\033[0m\n' ;
cd ./release/ ;
./MyTemplate && printf '\033[0;1;37m... done!\033[0m\n'
#eof
